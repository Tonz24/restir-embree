#include "stdafx.h"
#include "ReSTIRIntegrator.h"

#include <iostream>

#include "Intersection.h"
#include "Reservoir.h"
#include "Sampling.h"
#include "simpleguidx11.h"
#include "glm/gtx/string_cast.hpp"


int ReSTIRIntegrator::M_Area{ 1 };
int ReSTIRIntegrator::M_Brdf{ 1 };

int ReSTIRIntegrator::spatialReuseNeighborCount{ 5 };
int ReSTIRIntegrator::spatialPassCount{ 1 };
int ReSTIRIntegrator::confidenceCap{ 20};
float ReSTIRIntegrator::spatialReuseRadius{ 30};
float ReSTIRIntegrator::minNormalSimilarity{ 0.85 };
float ReSTIRIntegrator::maxDepthDifference{ 0.2 };


bool ReSTIRIntegrator::doSpatialReuse{ false };
bool ReSTIRIntegrator::doTemporalReuse{ false };
bool ReSTIRIntegrator::doUnbiasedTemporalMIS{ false };
bool ReSTIRIntegrator::doVisibilityPass{ false };
bool ReSTIRIntegrator::weighLastFrame{ false };
bool ReSTIRIntegrator::rejectDissimilarNeighbors{ false };
bool ReSTIRIntegrator::debugReprojection{ false };


ReSTIRIntegrator::SpatialWeightCalculation ReSTIRIntegrator::spatialWeightCalc{CONSTANT};

RenderParams ReSTIRIntegrator::renderParams{};

void ReSTIRIntegrator::gui2() {
	ImGui::DragInt("Area Reservoir samples", &M_Area, 1, 0, 2048);
	ImGui::DragInt("BRDF Reservoir samples", &M_Brdf, 1, 0, 2048);
	ImGui::Checkbox("Visibility pass", &doVisibilityPass);


	ImGui::DragInt("Confidence cap", &confidenceCap, 1, 1, 150);

	ImGui::Checkbox("Spatial reuse", &doSpatialReuse);
	if (doSpatialReuse) {
		ImGui::DragInt("Spatial pass count", &spatialPassCount, 1, 0, 2048);
		ImGui::DragInt("Spatial reuse neighbor count", &spatialReuseNeighborCount, 1, 0, 2048);
		ImGui::DragFloat("Spatial reuse radius", &spatialReuseRadius, 1.0, 0, 2048);

		static const char* items[]{ /*"Ray Tracer", */"Constant (1/M)","Constant (1/M) -- 1/|Z| debias","Constant (1/M) -- contrib. weight debias", "Balance Heuristic", "Pairwise MIS" };
		static int integratorIndex = 0;

		if (ImGui::Combo("MIS weight calculation", &integratorIndex, items, IM_ARRAYSIZE(items))) {
			switch (integratorIndex) {

			default:
			case 0:
				spatialWeightCalc = CONSTANT;
				break;
			case 1:
				spatialWeightCalc = CONSTANT_DEBIAS_Z_TERM;
				break;
			case 2:
				spatialWeightCalc = CONSTANT_DEBIAS_CONTRIB;
				break;
			case 3:
				spatialWeightCalc = BALANCE_HEURISTIC;
				break;
			case 4:
				spatialWeightCalc = PAIRWISE_MIS;
				break;
			
			}
		}
		ImGui::Checkbox("Reject dissimilar neighbors", &rejectDissimilarNeighbors);
		if (rejectDissimilarNeighbors){
			ImGui::DragFloat("Minimal dot product between normals (to accept)",&minNormalSimilarity,0.01,0.0,1.0);
			ImGui::DragFloat("Max depth difference (to accept)",&maxDepthDifference,0.01,0.0,1.0);
		}
	}

	ImGui::Checkbox("Temporal reuse", &doTemporalReuse);
	if (doTemporalReuse){
		ImGui::Checkbox("Unbiased MIS weights (otherwise 0.5 weights are used)", &doUnbiasedTemporalMIS);
		ImGui::Checkbox("Weigh last frame", &weighLastFrame);
		ImGui::Checkbox("Show occlusion / disocclusion", &debugReprojection);
	}
}

InitialCandidateSample ReSTIRIntegrator::areaSampleLight(const Scene& scene, const glm::vec<2, int>& pixelCoord){

	//pick a triangle
	auto triangleCDFSample = scene.getEmissiveCDF().getTriangle();
	//pick a point on that triangle
	auto trianglePoint = Sampling::sampleTriangle(triangleCDFSample.triangle);

	const GBufferElement& elem = SimpleGuiDX11::gBuffer.getAt(pixelCoord);

	const auto pdfEval = getMaterialPDFEvalFunc(elem.materialType);

	float pdf_area = triangleCDFSample.pdf * trianglePoint.pdf;

	glm::vec3 lightDir = trianglePoint.samplePoint - elem.worldSpacePos;
	float r_sqr = glm::dot(lightDir, lightDir);
	lightDir = glm::normalize(lightDir);

	float cosThetaY = glm::max(glm::dot(-lightDir, trianglePoint.normal),0.0f); //cosine between light sample normal and negative light direction
	float areaMeasureFactor = cosThetaY / r_sqr;

	float pdfAsIfBrdf = pdfEval(elem,SimpleGuiDX11::gBuffer.getCameraPos(),lightDir);
	float pdfAsIfBrdfAreaMeasure = pdfAsIfBrdf * areaMeasureFactor;

	Triangle* tri = const_cast<Triangle*>(&triangleCDFSample.triangle);

	LightSample s;
	
	s.samplePoint = trianglePoint.samplePoint;
	s.sampleNormal = trianglePoint.normal;
	s.L_i = triangleCDFSample.triangle.getSurface().get_material()->emission;
	
	float misWeight = m_area(pdf_area, pdfAsIfBrdfAreaMeasure);

	return {s,1.0f / pdf_area, misWeight};

}

InitialCandidateSample ReSTIRIntegrator::brdfSampleLight( const Scene& scene, const glm::vec<2, int>& pixelCoord) {
	const GBufferElement& elem = SimpleGuiDX11::gBuffer.getAt(pixelCoord);

	const auto pdfEval = getMaterialPDFEvalFunc(elem.materialType);
	const auto brdfSample = getMaterialSampleFunc(elem.materialType);

	glm::vec3 L_direct{ 0 };

	//calculate everything: generate a sample with corresponding pdf value, evaluate the brdf for it 
	//auto payloadGI = mat.evaluateLightingGI(hitInfo, ray);
	auto payloadGI = brdfSample(elem,SimpleGuiDX11::gBuffer.getCameraPos());

	//prepare a ray that goes in the sampled direction
	Ray brdfBounceRay{ elem.worldSpacePos + renderParams.normalOffset * elem.worldSpaceNormal,payloadGI.omega_i, FLT_MIN + renderParams.tnearOffset,FLT_MAX };
	//trace it
	auto bouncedIntersection = Intersection::intersectEmbree(scene, brdfBounceRay);

	LightSample s;
	float W{};
	float misWeight{};

	//the ray has to hit an emissive triangle
	if (bouncedIntersection.hitInfo.didHit && bouncedIntersection.material->isEmissive()) {

		glm::vec3 lightDir = bouncedIntersection.hitInfo.hitPoint - elem.worldSpacePos;
		float r_sqr = glm::dot(lightDir, lightDir);
		lightDir = glm::normalize(lightDir);

		float cosThetaY = glm::max(glm::dot(-lightDir, bouncedIntersection.hitInfo.normal), 0.0f);

		float areaMeasureFactor = cosThetaY / r_sqr;

		uint32_t triId = bouncedIntersection.hitInfo.hitTriId;
		Triangle* tri = scene.getEmissiveCDF().tris[triId];

		float brdfPdf = payloadGI.pdf;

		float pdf_area = scene.getEmissiveCDF().getPDFForTriangle(*tri);

		float brdfPdfAreaMeasure = brdfPdf * areaMeasureFactor;

		s.samplePoint = bouncedIntersection.hitInfo.hitPoint;
		s.sampleNormal = bouncedIntersection.hitInfo.normal;
		s.L_i = bouncedIntersection.material->emission;
		//s.valid = true;

		W = 1.0f / brdfPdfAreaMeasure;
		misWeight = m_brdf(brdfPdfAreaMeasure, pdf_area);
	}

	return { s,W, misWeight};
}


float ReSTIRIntegrator::evaluatePHat(const LightSample& sample, const Scene& scene, const glm::vec3& cameraPos, const
                                     GBufferElement& gBufferElem, bool testVisibility) {
	return glm::length(evaluateF(sample,scene,cameraPos, gBufferElem, testVisibility));
}

glm::vec3 ReSTIRIntegrator::evaluateF(const LightSample& sample, const Scene& scene, const glm::vec3& cameraPos, const
                                      GBufferElement& gBufferElem, bool testVisibility){

	if (!sample.isValid() || gBufferElem.emission.x > 0 || gBufferElem.emission.y > 0 || gBufferElem.emission.z > 0) return glm::vec3{ 0 };

	auto brdfEval = getMaterialBRDFEvalFunc(gBufferElem.materialType);

	glm::vec3 lightDir = sample.samplePoint - gBufferElem.worldSpacePos;
	float r_sqr = glm::dot(lightDir, lightDir);
	lightDir = glm::normalize(lightDir);

	float cosThetaI = glm::max(glm::dot(lightDir, gBufferElem.worldSpaceNormal), 0.0f); //cosine between hitpoint normal and light direction
	float cosThetaY = glm::abs(glm::dot(-lightDir, sample.sampleNormal)); //cosine between light sample normal and negative light direction

	float G = cosThetaI * cosThetaY / r_sqr;

	glm::vec3 f_r = brdfEval(gBufferElem, cameraPos, lightDir);

	bool V = true;

	if (testVisibility)
		V = !Intersection::testOcclusion(gBufferElem.worldSpacePos, sample.samplePoint, scene, renderParams);

	glm::vec3 L_direct = sample.L_i * f_r * G * static_cast<float>(V);

	return L_direct;
}

void ReSTIRIntegrator::gBufferFillPass(const Scene& scene, const glm::vec<2, int>& pixelCoords, const Camera& camera){
	Ray ray{ camera.GenerateRay(pixelCoords) };
	auto intResult = Intersection::intersectEmbree(scene, ray);
	GBufferElement elem{};

	if (intResult.hitInfo.didHit) {

		elem.worldSpacePos = intResult.hitInfo.hitPoint;
		elem.worldSpaceNormal = intResult.hitInfo.normal;
		elem.depth = glm::length(intResult.hitInfo.hitPoint - ray.getOrg());

		elem.materialType = intResult.material->getType();
		elem.diffuseColor = intResult.material->getDiffuseColor(intResult.hitInfo.uv);
		elem.specularColor = intResult.material->getSpecularColor(intResult.hitInfo.uv);
		elem.emission = intResult.material->emission;
		elem.shininess = intResult.material->getShininess(intResult.hitInfo.uv);
	}
	else
		elem.emission = renderParams.useSkybox ? scene.getSkybox().getTexel(ray.getDir()) : renderParams.bgColor;

	SimpleGuiDX11::gBuffer.setAt(pixelCoords, elem);
}

void ReSTIRIntegrator::initialRenderPass(const Scene& scene, const glm::vec<2, int>& pixelCoords) {

	glm::vec3 gBufferEmission = SimpleGuiDX11::gBuffer.getAt(pixelCoords).emission;
	bool isEmissive = gBufferEmission.x > 0 || gBufferEmission.y > 0 || gBufferEmission.z > 0;

	if (isEmissive || !scene.getEmissiveCDF().isValid()){
		SimpleGuiDX11::getReservoirWrite(pixelCoords) = Reservoir{};
		return;
	}

	Reservoir r{};

	if (pixelCoords == SimpleGuiDX11::debugPixel)
		int h = 10;

	if (M_Area > 0) {

		float inv_MArea = 1.0f / static_cast<float>(M_Area);
		for (int i = 0; i < M_Area; ++i) {
			InitialCandidateSample initialSample = areaSampleLight(scene, pixelCoords);
			LightSample& s = initialSample.sample;

			float p_hat = evaluatePHat(s, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(pixelCoords), !doVisibilityPass);

			float w{};
			if (M_Brdf > 0) {
				w = initialSample.misWeight * p_hat * initialSample.W;
			}
			else
				w = inv_MArea * p_hat * initialSample.W;
			r.addSample(s, w,1);
		}
	}

	if (M_Brdf > 0) {

		float inv_MBrdf = 1.0f / static_cast<float>(M_Brdf);
		for (int i = 0; i < M_Brdf; ++i) {
			InitialCandidateSample  initialSample = brdfSampleLight(scene, pixelCoords);
			LightSample& s = initialSample.sample;

			float p_hat = evaluatePHat(s, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(pixelCoords), !doVisibilityPass);

			float w{};
			if (M_Area > 0) {
				w = initialSample.misWeight * p_hat * initialSample.W;
			}
			else
				w = inv_MBrdf * p_hat * initialSample.W;
			r.addSample(s, w,1);
		}
	}

	float p_hat = evaluatePHat(r.bestSample, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(pixelCoords), !doVisibilityPass);
	r.W = p_hat > 0.0f ? 1.0f / p_hat * r.w_sum : 0.0f;

	//r.calculateW();
	r.capConfidence(confidenceCap);

	/*if (pixelCoords == SimpleGuiDX11::debugPixel)
		std::cout << "==============================\nconfidence at candidate generation pass " << r.confidence << std::endl;*/
	SimpleGuiDX11::getReservoirWrite(pixelCoords) = r;
}



void ReSTIRIntegrator::visibilityPass(const Scene& scene, const glm::vec<2, int>& pixelCoords){

	const glm::vec3& samplePoint = SimpleGuiDX11::getReservoirWrite(pixelCoords).bestSample.samplePoint;
	const glm::vec3& shadingPoint = SimpleGuiDX11::gBuffer.getAt(pixelCoords).worldSpacePos;

	bool V = !Intersection::testOcclusion(shadingPoint, samplePoint, scene, renderParams);

	if (!V)
		SimpleGuiDX11::getReservoirWrite(pixelCoords).W = 0;

}



void ReSTIRIntegrator::spatialReusePass(const glm::vec<2, int>& pixelCoords, const Scene& scene) {

	//early exit if this pixel is emissive as there are no shading calculations needed
	const GBufferElement& thisElem = SimpleGuiDX11::gBuffer.getAt(pixelCoords);
	bool isEmissive = thisElem.emission.x > 0 || thisElem.emission.y > 0 || thisElem.emission.z > 0;
	if (isEmissive) {
		SimpleGuiDX11::getReservoirWrite(pixelCoords) = SimpleGuiDX11::getReservoirRead(pixelCoords);;
		return;
	}

	std::vector<glm::vec<2, int>> neighborPixelCoords{};
	neighborPixelCoords.push_back(pixelCoords); //this pixel will always be resampled from

	//number of samples inspected
	//put 1 as the default value to automatically accomodate resampling from the center pixel
	int M{1};

	//select valid spatial neighbors
	for (int i = 0; i < spatialReuseNeighborCount; ++i){

		//uniformly sample a disk within selected radius
		//clamp the coordinates to screen
		glm::vec<2, int> neighborOffset = Sampling::sampleDiskUniform(spatialReuseRadius);
		glm::vec<2, int> neighborCoords = pixelCoords + neighborOffset;
		neighborCoords = glm::clamp(neighborCoords, { 0,0 }, { SimpleGuiDX11::width_ - 1,SimpleGuiDX11::height_ - 1 });

		const GBufferElement& neighborElem = SimpleGuiDX11::gBuffer.getAt(neighborCoords);

		//do not resample from emissive neighbors, they never have a valid reservoir
		if (neighborElem.emission.x > 0 || neighborElem.emission.y > 0 || neighborElem.emission.z > 0)
			continue;

		if (rejectDissimilarNeighbors) {
			float normalSimilarity{ glm::dot(neighborElem.worldSpaceNormal,thisElem.worldSpaceNormal) };

			//reject neighbor if the hit point normal is too dissimilar from current pixel's normal
			if (normalSimilarity < minNormalSimilarity)
				continue;

			float depthRatio{ 0 };
			if (neighborElem.depth > 0)
				depthRatio = thisElem.depth / neighborElem.depth;

			float halfDepthDiff = maxDepthDifference * 0.5f;
			float lowerDepthRatioBound = 1.0f - halfDepthDiff;
			float upperDepthRatioBound = 1.0f + halfDepthDiff;

			//reject neighbor if the ratio of depths between current pixel and the neighbor is too large / small
			if (depthRatio < lowerDepthRatioBound || depthRatio > upperDepthRatioBound)
				continue;

			neighborPixelCoords.push_back(neighborCoords);
			M += 1;
		}
		else {
			neighborPixelCoords.push_back(neighborCoords);
			M += 1;
		}
	}

	Reservoir resultReservoir{};


	int confidenceSum{ 0 },confidenceSumNonCanonical{0};
	for (int i = 0; i < neighborPixelCoords.size(); ++i){
		int confidence = SimpleGuiDX11::getReservoirRead(neighborPixelCoords[i]).confidence;
		confidenceSum += confidence;
		if (i != 0)
			confidenceSumNonCanonical += confidence;
	}

	int selectedSampleIndex{ 0 };
	float rcpM = M > 0 ? 1.0f / static_cast<float> (M) : 0.0f;

	for (int i = 0; i < neighborPixelCoords.size(); ++i){

		glm::vec<2, int> coords_i = neighborPixelCoords[i];
		const Reservoir& reservoir_i = SimpleGuiDX11::getReservoirRead(coords_i);
		const LightSample& sample_i = reservoir_i.bestSample;

		/*  GENERALIZED BALANCE HEURISTIC MIS
		 * for evaluating the MIS weight, we need to evaluate the proxy pdf (p_hat) for this sample (i) as if it came from all the other neighbors (js)
		 * that is, evaluate p_hat for sample i with world space position every j sample
		 * then compute the balance heuristic MIS weight
		 * the complexity is O(M^2) since the inner loop (over js) needs to be calculated for every neighbor (every i)
		 * 
		 * 1/M MIS weights are faster to compute (it's just a constant value), but they lead to bias, unless debiasing is applied
		 * debiasing is done only for the surviving sample, hence the complexity drops to O(M).
		 * In the context of just direct lighting, the simple debiasing techniques don't lead to much variance, but when doing GI with ReSTIR, it's preferable to use more sophisticated methods like pairwise MIS (at least the paper says so)
		 */
		float misNom{0}, misDenom{0}, misWeight{ rcpM };
		if (spatialWeightCalc == BALANCE_HEURISTIC) {
			misWeight = 0.0f;
			for (int j = 0; j < neighborPixelCoords.size() ; ++j){

				glm::vec<2, int> coords_j = neighborPixelCoords[j];
				const Reservoir& reservoir_j = SimpleGuiDX11::getReservoirRead(coords_j);

				float p_hat = evaluatePHat(sample_i, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(coords_j), true);

				//p_hat of sample i from its corresponding world space position is the nominator ("pdf" of sampling this light from its world space position)
				//denominator is the sum of sampling i from the world space position of every selected neighbor
				misDenom += p_hat * reservoir_j.confidence;
				if (i == j) 
					misNom = p_hat * reservoir_i.confidence;
			}
			if (misDenom > 0)
				misWeight = misNom / misDenom;
		}


		/* PAIRWISE MIS
		 */
		/*if (spatialWeightCalc == PAIRWISE_MIS) {
			misWeight = 0.0f;
			float MminusOne = static_cast<float>(M - 1);

			//canonical sample weight
			//the canonical sample is always at index 0 
			if (i == 0){ 

				float sum{ 0.0f };
				float p_hat_c = evaluatePHat(sample_i, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(coords_i), true);
				float p_hat_c_div = p_hat_c / MminusOne;

				for (int j = 0; j < neighborPixelCoords.size(); ++j){

					if (i == j)
						continue;

					glm::vec<2, int> coords_j = neighborPixelCoords[j];
					float p_hat_j = evaluatePHat(sample_i, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(coords_j), true);

					float denom = p_hat_c_div + p_hat_j;

					if (denom > 0)
						sum += p_hat_c_div / denom;
				}
				misWeight = rcpM * (1.0f + sum);
			}

			//non canonical sample weight
			else {
				float p_hat_i = evaluatePHat(sample_i, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(coords_i), true);
				float p_hat_c = evaluatePHat(sample_i, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(neighborPixelCoords[0]), true);

				float denom = p_hat_i + p_hat_c / MminusOne;

				if (denom > 0)
					misWeight = rcpM * (p_hat_i / denom);
			}
		}*/

		if (spatialWeightCalc == PAIRWISE_MIS) {
			misWeight = 0.0f;
			float MminusOne = static_cast<float>(M - 1);

			//canonical sample weight
			//the canonical sample is always at index 0 
			if (i == 0) {

				float sum{ 0.0f };
				float p_hat_c = evaluatePHat(sample_i, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(coords_i), true) * static_cast<float>(reservoir_i.confidence);


				for (int j = 1; j < neighborPixelCoords.size(); ++j) {

					glm::vec<2, int> coords_j = neighborPixelCoords[j];
					const Reservoir& reservoir_j = SimpleGuiDX11::getReservoirRead(coords_j);
					float p_hat_j = evaluatePHat(sample_i, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(coords_j), true);

					float denom = p_hat_c + p_hat_j * static_cast<float>(confidenceSumNonCanonical);

					if (denom > 0) {
						float confFract = static_cast<float>(reservoir_j.confidence) / static_cast<float>(confidenceSum);
						sum += confFract *(p_hat_c / denom);
					}
				}
				misWeight = (static_cast<float>(reservoir_i.confidence) / static_cast<float>(confidenceSum)) + sum;
			}

			//non canonical sample weight
			else {
				float p_hat_i = evaluatePHat(sample_i, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(coords_i), true);
				float p_hat_c = evaluatePHat(sample_i, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(neighborPixelCoords[0]), true);

				p_hat_i *= static_cast<float> (confidenceSumNonCanonical);

				float denom = p_hat_i + p_hat_c * static_cast<float>(SimpleGuiDX11::getReservoirRead(neighborPixelCoords[0]).confidence);

				if (denom > 0 && confidenceSum > 0)
					misWeight = (static_cast<float>(reservoir_i.confidence) / static_cast<float>(confidenceSum)) * (p_hat_i / denom);
			}
		}


		//calculate the resampling weight -> same logic as when generating initial candidates
		//evaluate p_hat for this pixel's world space hit point and sample i -> higher values of p_hat result in higher probability of sample i being selected
		float resamplingPhat = evaluatePHat(sample_i, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(pixelCoords), true);
		float resamplingWeight = misWeight * resamplingPhat * reservoir_i.W;

		//save the surviving sample index for contribution weight debiasing
		if (resultReservoir.addSample(sample_i, resamplingWeight, reservoir_i.confidence))
			selectedSampleIndex = i;
	}

	//finalize resampling -> calculate UCW
	float final_p_hat = evaluatePHat(resultReservoir.bestSample, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(pixelCoords), true);

	if (spatialWeightCalc == CONSTANT || spatialWeightCalc == BALANCE_HEURISTIC || spatialWeightCalc == PAIRWISE_MIS)
	{
		resultReservoir.W = final_p_hat > 0.0f ? resultReservoir.w_sum / final_p_hat : 0.0f;
	}

	/*
	 * multiply the UCW by correction factor (1/Z) / (1/M)
	 * Z = number of techniques (neighbors) with non-zero PDF for the selected output sample
	 * M = number of neighbors partaking in the resampling process
	 * This is an O(M) algorithm, since the Z value needs to only be calculated for the sample that survived the resampling process
	 */
	else if (spatialWeightCalc == CONSTANT_DEBIAS_Z_TERM)
	{
		int Z{ 0 };
		float correctionFactor{ 1.0f };
		for (int i = 0; i < neighborPixelCoords.size(); ++i) {
			if (!Intersection::testOcclusion(SimpleGuiDX11::gBuffer.getAt(neighborPixelCoords[i]).worldSpacePos, resultReservoir.bestSample.samplePoint, scene, renderParams))
				Z += 1;
		}
		if (Z >	0 && M > 0)
			correctionFactor = (1.0f / static_cast<float>(Z)) / rcpM;

		resultReservoir.W = final_p_hat > 0.0f ? correctionFactor * resultReservoir.w_sum / final_p_hat : 0.0f;
	}


	/*
	 * multiply the UCW by correction factor (Contribution weight) / (1/M)
	 * Contribution weight = correct MIS weight for the selected sample
	 * M = number of neighbors partaking in the resampling process
	 * This is an O(M) algorithm, since the proper MIS weight needs to only be calculated for the sample that survived the resampling process
	 */
	else if (spatialWeightCalc == CONSTANT_DEBIAS_CONTRIB)
	{
		const glm::vec<2, int>& selectedSampleCoords = neighborPixelCoords[selectedSampleIndex];
		const LightSample& selectedSample = SimpleGuiDX11::getReservoirRead(selectedSampleCoords).bestSample;

		float misNom{ 0 }, misDenom{ 0 }, contribWeight{0}, correctionFactor{0};

		for (int i = 0; i < neighborPixelCoords.size(); ++i) {

			glm::vec<2, int> coords_i = neighborPixelCoords[i];
			const Reservoir& reservoir_i = SimpleGuiDX11::getReservoirRead(coords_i);
			float p_hat = evaluatePHat(selectedSample, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(coords_i), true);

			misDenom += p_hat * static_cast<float>(reservoir_i.confidence);
			if (i == selectedSampleIndex)
				misNom = p_hat * static_cast<float>(reservoir_i.confidence);
		}
		if (misDenom > 0)
			contribWeight = misNom / misDenom;
		if (M > 0)
			correctionFactor = contribWeight / rcpM;

		resultReservoir.W = final_p_hat > 0.0f ? correctionFactor * resultReservoir.w_sum / final_p_hat : 0.0f;
	}

	resultReservoir.capConfidence(confidenceCap);
	SimpleGuiDX11::getReservoirWrite(pixelCoords) = resultReservoir;
}

glm::vec<2, int> ReSTIRIntegrator::reprojectBackward(const glm::vec3& wsPos, const glm::vec<2, int>& pixelCoords) {

	// Homogeneous coordinates
	glm::vec4 viewSpacePosH = SimpleGuiDX11::gBufferLastFrame.getViewMat() * glm::vec4(wsPos,1.0f);
	glm::vec3 viewSpacePos = glm::vec3(viewSpacePosH);

	// if z > 0 in view space, it means that the point lies BEHIND the camera, which makes it invalid
	// -z is the forward direction
	if (viewSpacePos.z >= 0) 
		return INVALID_REPROJECTION;

	// Calculate focal plane coordinates
	float focal = SimpleGuiDX11::gBufferLastFrame.getFocalLength();
	int screenX = glm::round((-viewSpacePos.x / viewSpacePos.z) * focal + static_cast<float>(SimpleGuiDX11::width_) / 2.0f);
	int screenY = glm::round((viewSpacePos.y / viewSpacePos.z) * focal + static_cast<float>(SimpleGuiDX11::height_) / 2.0f);

	//the reprojection is invalid if the reprojected pixel coordinates lie outside the screen
	if (screenX < 0 || screenX > SimpleGuiDX11::width_ - 1 || screenY < 0 || screenY > SimpleGuiDX11::height_ - 1)
		return INVALID_REPROJECTION;

	return { screenX, screenY };
}

glm::vec<2, int> ReSTIRIntegrator::reprojectForward(const glm::vec3& wsPos, const glm::vec<2, int>& pixelCoords) {
	// Homogeneous coordinates
	glm::vec4 viewSpacePosH = SimpleGuiDX11::gBuffer.getViewMat() * glm::vec4(wsPos, 1.0f);
	glm::vec3 viewSpacePos = glm::vec3(viewSpacePosH);

	// if z > 0 in view space, it means that the point lies BEHIND the camera, which makes it invalid
	// -z is the forward direction
	if (viewSpacePos.z >= 0)
		return INVALID_REPROJECTION;

	// Calculate focal plane coordinates
	float focal = SimpleGuiDX11::gBuffer.getFocalLength();
	int screenX = glm::round((-viewSpacePos.x / viewSpacePos.z) * focal + static_cast<float>(SimpleGuiDX11::width_) / 2.0f);
	int screenY = glm::round((viewSpacePos.y / viewSpacePos.z) * focal + static_cast<float>(SimpleGuiDX11::height_) / 2.0f);

	//the reprojection is invalid if the reprojected pixel coordinates lie outside the screen
	if (screenX < 0 || screenX > SimpleGuiDX11::width_ - 1 || screenY < 0 || screenY > SimpleGuiDX11::height_ - 1)
		return INVALID_REPROJECTION;

	return { screenX, screenY };
}



bool ReSTIRIntegrator::testReprojection() {
	for (int x = 0; x < SimpleGuiDX11::width_; ++x) {
		for (int y = 0; y < SimpleGuiDX11::height_; ++x) {

			const glm::vec3& wsPos = SimpleGuiDX11::gBuffer.getAt({ x,y }).worldSpacePos;

			// Homogeneous coordinates
			glm::vec4 viewSpacePosH = SimpleGuiDX11::gBuffer.getViewMat() * glm::vec4(wsPos, 1.0f);
			glm::vec3 viewSpacePos = glm::vec3(viewSpacePosH);

			// if z > 0 in view space, it means that the point lies BEHIND the camera, which makes it invalid
			// -z is the forward direction
			if (viewSpacePos.z >= 0)
				return false;

			// Calculate reprojected focal plane coordinates in pixels
			float focal = SimpleGuiDX11::gBuffer.getFocalLength();
			int screenX = glm::round((-viewSpacePos.x / viewSpacePos.z) * focal + static_cast<float>(SimpleGuiDX11::width_) / 2.0f);
			int screenY = glm::round((viewSpacePos.y / viewSpacePos.z) * focal + static_cast<float>(SimpleGuiDX11::height_) / 2.0f);

			//the reprojection is invalid if the reprojected pixel coordinates lie outside the screen
			if (screenX < 0 || screenX > SimpleGuiDX11::width_ - 1 || screenY < 0 || screenY > SimpleGuiDX11::height_ - 1)
				return false;

			// the reprojected screen coordinates MUST match the original screen coordinates
			if (screenX != x || screenY != y)
				return false;
		}
	}
	//all reprojected coordinates have been successfully matched with their original pixel coordinates
	return true;
}


void ReSTIRIntegrator::temporalReusePass(const glm::vec<2, int>& pixelCoords, const Scene& scene, const Camera& camera) {


	if (pixelCoords == SimpleGuiDX11::debugPixel || pixelCoords == glm::vec < 2, int>{ 90,200 })
		int h = 10;

	Reservoir resultReservoir{};

	const GBufferElement& currentElem = SimpleGuiDX11::gBuffer.getAt(pixelCoords);
	const glm::vec3& currentCamPos = SimpleGuiDX11::gBuffer.getCameraPos();

	glm::vec<2,int> prevFramePixelCoords = reprojectBackward(currentElem.worldSpacePos, pixelCoords);

	const Reservoir& currentReservoir = SimpleGuiDX11::getReservoirRead(pixelCoords);
	const LightSample& currentSample = currentReservoir.bestSample;

	const Reservoir& prevReservoir = SimpleGuiDX11::getReservoirLastFrame(pixelCoords);
	const LightSample& prevSample = prevReservoir.bestSample;

	if (prevFramePixelCoords == INVALID_REPROJECTION){
		SimpleGuiDX11::getReservoirWrite(pixelCoords) = currentReservoir;

		if (debugReprojection)
			SimpleGuiDX11::gBuffer.emissionBuf[pixelCoords.y * SimpleGuiDX11::width_ + pixelCoords.x] = {100,100,0};
		return;
	}

	const GBufferElement& prevElem = SimpleGuiDX11::gBufferLastFrame.getAt(prevFramePixelCoords);
	const glm::vec3& prevCamPos = SimpleGuiDX11::gBufferLastFrame.getCameraPos();

	float currentDepth = glm::length(currentElem.worldSpacePos - currentCamPos);
	float prevDepth = glm::length(prevElem.worldSpacePos - prevCamPos);

	float depthRatio = currentDepth > prevDepth ? prevDepth / currentDepth :  currentDepth/ prevDepth;

	if (depthRatio < 0.9f){
		SimpleGuiDX11::getReservoirWrite(pixelCoords) = currentReservoir;
		if (debugReprojection)
			SimpleGuiDX11::gBuffer.emissionBuf[pixelCoords.y * SimpleGuiDX11::width_ + pixelCoords.x] = { 0,100,0 };
		return;
	}


	const auto& prevElemAtCurrent = SimpleGuiDX11::gBufferLastFrame.getAt(pixelCoords);
	const auto& prevReprojected = reprojectForward(prevElemAtCurrent.worldSpacePos,pixelCoords);

	if (prevReprojected == INVALID_REPROJECTION) {
		SimpleGuiDX11::getReservoirWrite(pixelCoords) = currentReservoir;

		if (debugReprojection)
			SimpleGuiDX11::gBuffer.emissionBuf[pixelCoords.y * SimpleGuiDX11::width_ + pixelCoords.x] = { 100,0,100 };
		return;
	}

	const auto& fwReprojected = SimpleGuiDX11::gBuffer.getAt(prevReprojected);
	float currentDepthP = glm::length(prevElemAtCurrent.worldSpacePos - prevCamPos);
	float prevDepthP = glm::length(fwReprojected.worldSpacePos - currentCamPos);

	float depthRatioP = currentDepthP > prevDepthP ? prevDepthP / currentDepthP : currentDepthP / prevDepthP;


	if (depthRatioP < 0.9f) {
		SimpleGuiDX11::getReservoirWrite(pixelCoords) = currentReservoir;
		if (debugReprojection)
			SimpleGuiDX11::gBuffer.emissionBuf[prevReprojected.y * SimpleGuiDX11::width_ + prevReprojected.x] = { 0,0,100 };
		return;
	}


	//calculate m_cur(x)
	float p_cur = evaluatePHat(currentSample, scene, currentCamPos, currentElem, true);
	float p_prev = evaluatePHat(currentSample, scene, prevCamPos, prevElem, true);

	//float m_cur = (p_cur * static_cast<float>(cur_res.samplesProcessed) ) / (static_cast<float>(cur_res.samplesProcessed) * p_cur + static_cast<float>(prev_res.samplesProcessed) * p_prev);
	//float m_cur = p_cur / (p_cur + p_prev);
	float m_cur = p_cur * static_cast<float>(currentReservoir.confidence) / (p_cur * static_cast<float>(currentReservoir.confidence) + p_prev * static_cast<float>(prevReservoir.confidence));
	//float m_cur = p_cur / (p_cur +  20.0f * p_prev);

	if (!(m_cur > 0))
		m_cur = 0.0f;

	float p_hat_cur = evaluatePHat(currentSample, scene, currentCamPos, currentElem, true);
	float w_cur = m_cur * p_hat_cur * currentReservoir.W;
	resultReservoir.addSample(currentSample, w_cur, currentReservoir.confidence);

	p_cur = evaluatePHat(prevSample, scene, currentCamPos, currentElem, true);
	p_prev = evaluatePHat(prevSample, scene, prevCamPos, prevElem, true);

	//float m_prev = (static_cast<float>(prev_res.samplesProcessed) * p_prev) / (static_cast<float>(cur_res.samplesProcessed) * p_cur + static_cast<float>(prev_res.samplesProcessed) * p_prev);
	//float m_prev = p_prev / (p_cur + p_prev);
	float m_prev = p_prev * static_cast<float>(prevReservoir.confidence) / (p_cur * static_cast<float>(currentReservoir.confidence) + p_prev * static_cast<float>(prevReservoir.confidence));
	//float m_prev = 20.0f * p_prev / (p_cur + 20.0f * p_prev);

	if (!(m_prev > 0))
		m_prev = 0.0f;

	float p_hat_prev = evaluatePHat(prevSample, scene, currentCamPos, currentElem, true);
	float w_prev = m_prev * p_hat_prev * prevReservoir.W;
	resultReservoir.addSample(prevSample, w_prev, prevReservoir.confidence);

	resultReservoir.capConfidence(confidenceCap);

	float final_p_hat = evaluatePHat(resultReservoir.bestSample, scene, SimpleGuiDX11::gBuffer.getCameraPos(), SimpleGuiDX11::gBuffer.getAt(pixelCoords), true);
	resultReservoir.W = final_p_hat > 0.0f ? resultReservoir.w_sum / final_p_hat : 0.0f;
	//resultReservoir.samplesProcessed = cur_res.samplesProcessed + prev_res.samplesProcessed;

	SimpleGuiDX11::getReservoirWrite(pixelCoords) = resultReservoir;
}
