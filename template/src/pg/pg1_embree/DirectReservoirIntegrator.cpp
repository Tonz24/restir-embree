#include "stdafx.h"
#include "DirectReservoirIntegrator.h"

#include "Intersection.h"
#include "Sampling.h"
#include "simpleguidx11.h"

void DirectReservoirIntegrator::drawGui() {
	ImGui::DragInt("Area Reservoir samples", &M_Area, 1, 0, 2048);
	ImGui::DragInt("BRDF Reservoir samples", &M_Brdf, 1, 0, 2048);
	ImGui::Checkbox("Spatial reuse", &doReuse);

	if (doReuse) {
		ImGui::DragInt("Spatial reuse neighbor count", &spatialReuseNeighborCount, 1, 0, 2048);
		ImGui::DragFloat("Spatial reuse radius", &spatialReuseRadius, 0.01, 0, 2048);
	}
}

glm::vec3 DirectReservoirIntegrator::calculateDirectLighting(const Scene& scene, const Ray& ray,
                                                             const IntersectionResult& intersection, const RenderParams& renderParams, const glm::vec<2,int>& pixelCoords) {

	glm::vec3 L_direct{ 0 };

	auto renderPass = SimpleGuiDX11::getCurrentRenderPass();
	initialRenderPass(scene, ray, intersection, renderParams, pixelCoords);

	if (doReuse){
		spatialReusePass(scene, ray, intersection, renderParams, pixelCoords);
	}

	Reservoir& r = SimpleGuiDX11::getReservoirWrite(pixelCoords);

	//L_direct = r.bestSample.L * r.W;

	return L_direct;
}

void DirectReservoirIntegrator::initialRenderPass(const Scene& scene, const Ray& ray,
	const IntersectionResult& intersection, const RenderParams& renderParams, const glm::vec<2, int>& pixelCoords) {

	Reservoir r{};
	if (!scene.getEmissiveCDF().isValid()) return;

 	/*if (M_Area > 0) {

		float inv_MArea = 1.0f / static_cast<float>(M_Area);
		for (int i = 0; i < M_Area; ++i) {

			LightSample s = areaSampleLight(ray, intersection, scene, renderParams);

			if (s.V)
				int h = 10;

			float w{};
			if (M_Brdf > 0) {
				float mArea = m_area(s.pdf_area, s.pdf_brdf);
				w = mArea * s.p_hat * s.W;
			}
			else
				w = inv_MArea * s.p_hat * s.W;
			r.addSample(s, w);
		}
	}

	if (M_Brdf > 0) {

		float inv_MBrdf = 1.0f / static_cast<float>(M_Brdf);
		for (int i = 0; i < M_Brdf; ++i) {

			LightSample s = brdfSampleLight(ray, intersection, scene, renderParams);

			float w{};
			if (M_Area > 0) {
				float mBrdf = m_brdf(s.pdf_brdf, s.pdf_area);
				w = mBrdf * s.p_hat * s.W;
			}
			else
				w = inv_MBrdf * s.p_hat * s.W;
			r.addSample(s, w);
		}
	}
	r.calculateW();

	Reservoir& reservoir = SimpleGuiDX11::getReservoirWrite(pixelCoords);
	reservoir = r;*/
}

void DirectReservoirIntegrator::spatialReusePass(const Scene& scene, const Ray& ray,
	const IntersectionResult& intersection, const RenderParams& renderParams, const glm::vec<2, int>& pixelCoords){

	//Reservoir& pixelReservoir = SimpleGuiDX11::getReservoirRead(pixelCoords);
	Reservoir pixelReservoir{};

	/*float invCount = 1.0f / static_cast<float>(spatialReuseNeighborCount);
	for (int i = 0; i < spatialReuseNeighborCount; ++i){

		glm::vec2 diskSample = Sampling::sampleDiskUniform(spatialReuseRadius,1);

		glm::vec<2,int> sampledCoords = pixelCoords + static_cast<glm::vec<2, int>>(diskSample);

		sampledCoords.x = glm::clamp(sampledCoords.x, 0, SimpleGuiDX11::width_ - 1);
		sampledCoords.y = glm::clamp(sampledCoords.y, 0, SimpleGuiDX11::height_ - 1);

		Reservoir& neighborReservoir = SimpleGuiDX11::getReservoirRead(sampledCoords);



		//float w = invCount * neighborReservoir.bestSample.p_hat * neighborReservoir.W;
		//pixelReservoir.addSample(neighborReservoir.bestSample, w);
	}

	pixelReservoir.calculateW();

	Reservoir& reservoir = SimpleGuiDX11::getReservoirWrite(pixelCoords);
	reservoir = pixelReservoir;*/
}

LightSample DirectReservoirIntegrator::areaSampleLight(const Ray& ray, const IntersectionResult& intResult, const Scene& scene,
                                                       const RenderParams& renderParams) {

	const HitInfo& hitInfo = intResult.hitInfo;
	const Material& mat = *intResult.material;

	//pick a triangle
	auto triangleCDFSample = scene.getEmissiveCDF().getTriangle();
	//pick a point on that triangle
	auto trianglePoint = Sampling::sampleTriangle(triangleCDFSample.triangle);

	LightSample s;

	float pdf_area = triangleCDFSample.pdf * trianglePoint.pdf;

	glm::vec3 lightDir = trianglePoint.samplePoint - hitInfo.hitPoint;
	float r_sqr = glm::dot(lightDir, lightDir);
	lightDir = glm::normalize(lightDir);

	float cosThetaI = glm::max(glm::dot(lightDir, hitInfo.normal), 0.0f); //cosine between hitpoint normal and light direction
	float cosThetaY = glm::abs(glm::dot(-lightDir, trianglePoint.normal)); //cosine between light sample normal and negative light direction
	float areaMeasureFactor = cosThetaY / r_sqr;

	float pdfAsIfBrdf = mat.getPdfForSample(hitInfo, ray, lightDir);
	float pdfAsIfBrdfAreaMeasure = pdfAsIfBrdf * areaMeasureFactor;

	glm::vec3 L_i = triangleCDFSample.triangle.getSurface().get_material()->emission;
	float G = cosThetaI * cosThetaY / r_sqr;
	glm::vec3 f_r = mat.evaluateBRDF(hitInfo, ray, lightDir).f_r;

	bool V = !Intersection::testOcclusion(hitInfo.hitPoint, trianglePoint.samplePoint, scene, renderParams);

	/*s.pdf_area = pdf_area;
	s.pdf_brdf = pdfAsIfBrdfAreaMeasure;
	s.W = 1.0f / s.pdf_area;*/

	return s;
}

LightSample DirectReservoirIntegrator::brdfSampleLight(const Ray& ray, const IntersectionResult& intResult, const Scene& scene,
	const RenderParams& renderParams) {

	const HitInfo& hitInfo = intResult.hitInfo;
	const Material& mat = *intResult.material;

	glm::vec3 L_direct{ 0 };

	//calculate everything: generate a sample with corresponding pdf value, evaluate the brdf for it 
	auto payloadGI = mat.evaluateLightingGI(hitInfo, ray);

	//prepare a ray that goes in the sampled direction
	Ray brdfBounceRay{ hitInfo.hitPoint + renderParams.normalOffset * hitInfo.normal,payloadGI.omega_i, FLT_MIN + renderParams.tnearOffset,FLT_MAX };
	//trace it
	auto bouncedIntersection = Intersection::intersectEmbree(scene, brdfBounceRay);

	LightSample s;

	//the ray has to hit an emissive triangle
	if (bouncedIntersection.hitInfo.didHit && bouncedIntersection.material->isEmissive()) {
		glm::vec3 L_i = bouncedIntersection.material->emission;

		glm::vec3 lightDir = bouncedIntersection.hitInfo.hitPoint - hitInfo.hitPoint;
		float r_sqr = glm::dot(lightDir, lightDir);
		lightDir = glm::normalize(lightDir);

		float cosThetaI = glm::max(glm::dot(lightDir, hitInfo.normal), 0.0f);
		float cosThetaY = glm::max(glm::dot(-lightDir, bouncedIntersection.hitInfo.normal), 0.0f);

		//float angleMeasureFactor = r_sqr / cosThetaY;
		float areaMeasureFactor = cosThetaY / r_sqr;

		uint32_t triId = bouncedIntersection.hitInfo.hitTriId;
		const Triangle& tri = *scene.getEmissiveCDF().tris[triId];

		float brdfPdf = payloadGI.pdf;

		float G = cosThetaI * cosThetaY / r_sqr;

		//should give same results
		L_direct = L_i * payloadGI.f_r * G;
		//L_direct = L_i * payloadGI.f_r * cosThetaI;

		/*s.pdf_area = pdf_area;
		s.pdf_brdf = brdfPdfAreaMeasure;
		s.W = 1.0f / s.pdf_brdf;*/
	}
	return s;
}