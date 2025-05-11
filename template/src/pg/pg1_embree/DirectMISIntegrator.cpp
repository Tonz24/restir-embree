#include "stdafx.h"
#include "DirectMISIntegrator.h"

#include <iostream>

#include "Intersection.h"
#include "Sampling.h"
#include "glm/gtx/string_cast.hpp"

float DirectMISIntegrator::powerHeuristic(float pdf, float pdfOther) {
	const float pdf_sqr = pdf * pdf;
	const float pdfOther_sqr = pdfOther * pdfOther;
	const float result = pdf_sqr / (pdfOther_sqr + pdf_sqr);
	return result;
}


glm::vec3 DirectMISIntegrator::calculateDirectLighting(const Scene& scene, const Ray& ray, const IntersectionResult& intersection, const RenderParams& renderParams, const glm
                                                       ::vec<2,int>& pixelCoords) {

	glm::vec3 L_direct{ 0 };

	if (sampleBRDF)
		L_direct += evaluateBRDFSample(scene, intersection, ray, renderParams);

	if (sampleLightSources)
		L_direct += evaluateLightSample(scene, intersection, ray, renderParams);

	return L_direct;
}
void DirectMISIntegrator::drawGui() {
	ImGui::Checkbox("Sample BRDF", &sampleBRDF);
	ImGui::Checkbox("Sample Light Sources", &sampleLightSources);
	ImGui::Checkbox("Show weights", &showWeights);
}


glm::vec3 DirectMISIntegrator::evaluateLightSample(const Scene& scene, const IntersectionResult& intersection, const Ray& ray, const RenderParams& renderParams) const
{
	const HitInfo& hitInfo = intersection.hitInfo;
	const Material& mat = *intersection.material;

	float misWeight{ 0 };
	glm::vec3 L_direct{ 0 };

	//there has to be at least one valid emissive triangle
	if (scene.getEmissiveCDF().isValid()){
		glm::vec3 L_i{ 0 };

		//pick a triangle relative to the surface area CDF
		auto triangleCDFSample = scene.getEmissiveCDF().getTriangle();

		//pick a point on that selected triangle
		auto trianglePoint = Sampling::sampleTriangle(triangleCDFSample.triangle);
		float lightPdf = triangleCDFSample.pdf * trianglePoint.pdf;

		if (lightPdf == 0) return glm::vec3{ 0 };

		//calculate radius squared and direction from x to y, used in geometry term calculation
		glm::vec3 lightDir = trianglePoint.samplePoint - hitInfo.hitPoint;
		float r_sqr = glm::dot(lightDir, lightDir);
		lightDir = glm::normalize(lightDir);

		if (r_sqr == 0) return glm::vec3{ 0.0f };

		float cosThetaI = glm::max(glm::dot(lightDir, hitInfo.normal), 0.0f); //cosine between hitpoint normal and light direction
		float cosThetaY = glm::max(glm::dot(-lightDir, trianglePoint.normal),0.0f); //cosine between light sample normal and negative light direction

		//float angleMeasureFactor = r_sqr / cosThetaY;
		float areaMeasureFactor = cosThetaY / r_sqr;

		//proceeding further makes sense only when we're going to receive non-zero radiance
		if (cosThetaI > 0 && cosThetaY > 0 && !Intersection::testOcclusion(hitInfo.hitPoint, trianglePoint.samplePoint, scene, renderParams)) {
			float pdfAsIfBrdf = mat.getPdfForSample(hitInfo, ray, lightDir);
			float pdfAsIfBrdfAreaMeasure = pdfAsIfBrdf * areaMeasureFactor;

			L_i = triangleCDFSample.triangle.getSurface().get_material()->emission;
			misWeight = powerHeuristic(lightPdf, pdfAsIfBrdfAreaMeasure);

			if (showWeights)
				return { 0,misWeight,0 };

			//evaluate for direct illumination
			if (misWeight > 0.0f){
				float G = cosThetaI * cosThetaY / r_sqr;
				glm::vec3 f_r = mat.evaluateBRDF(hitInfo, ray, lightDir).f_r;
				L_direct = misWeight * L_i * f_r * G / lightPdf;
			}
		}
	}
	return L_direct;
}

glm::vec3 DirectMISIntegrator::evaluateBRDFSample(const Scene& scene, const IntersectionResult& intersection, const Ray& ray, const RenderParams& renderParams) const {

	const HitInfo& hitInfo = intersection.hitInfo;
	const Material& mat = *intersection.material;

	float misWeight{ 0 };
	glm::vec3 L_direct{ 0 };

	//calculate everything: generate a sample with corresponding pdf value, evaluate the brdf for it 
	auto payloadGI = mat.evaluateLightingGI(hitInfo, ray);

	//prepare a ray that goes in the sampled direction
	Ray brdfBounceRay{ hitInfo.hitPoint + renderParams.normalOffset * hitInfo.normal,payloadGI.omega_i, FLT_MIN + renderParams.tnearOffset,FLT_MAX };
	//trace it
	auto bouncedIntersection = Intersection::intersectEmbree(scene, brdfBounceRay);

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

		float pdfAsIfLight = scene.getEmissiveCDF().getPDFForTriangle(tri);

		float brdfPdfAreaMeasure = brdfPdf * areaMeasureFactor;
		misWeight = powerHeuristic(brdfPdfAreaMeasure, pdfAsIfLight);

		if (showWeights)
			return { misWeight,0,0 };

		float G = cosThetaI * cosThetaY / r_sqr;

		//should give same results
		//L_direct = misWeight * L_i * payloadGI.f_r * G / brdfPdfAreaMeasure;
		L_direct = misWeight * L_i * payloadGI.f_r * cosThetaI / brdfPdf;
	}
	return L_direct;
}