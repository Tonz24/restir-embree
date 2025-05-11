#include "stdafx.h"
#include "DirectBRDFIntegrator.h"


#include "Intersection.h"

glm::vec3 DirectBRDFIntegrator::calculateDirectLighting(const Scene& scene, const Ray& ray, const IntersectionResult& intersection, const RenderParams& renderParams, const glm
                                                        ::vec<2,int>& pixelCoords) {
	const HitInfo& hitInfo = intersection.hitInfo;
	const Material& mat = *intersection.material;

	glm::vec3 L_direct{ 0 };

	//calculate everything: generate a sample with corresponding pdf value, evaluate the brdf for it 
	auto payloadGI = mat.evaluateLightingGI(hitInfo, ray);

	//prepare a ray that goes in the sampled direction
	Ray brdfBounceRay{ hitInfo.hitPoint + renderParams.normalOffset * hitInfo.normal,payloadGI.omega_i, FLT_MIN + renderParams.tnearOffset,FLT_MAX };
	//trace it
	auto bouncedIntersection = Intersection::intersectEmbree(scene, brdfBounceRay);
	//the ray has to hit an emissive triangle, otherwise L_o = 0 => MIS weight must also be 0
	if (bouncedIntersection.hitInfo.didHit && bouncedIntersection.material->isEmitter()) {
		glm::vec3 L_i = bouncedIntersection.material->emission;

		glm::vec3 omega_i = bouncedIntersection.hitInfo.hitPoint - hitInfo.hitPoint;
		float r_sqr = glm::dot(omega_i, omega_i);
		omega_i = glm::normalize(omega_i);

		if (r_sqr == 0) return glm::vec3{ 0.0f };

		float cosThetaI = glm::max(glm::dot(omega_i, hitInfo.normal), 0.0f);
		float cosThetaY = glm::max(glm::dot(-omega_i, bouncedIntersection.hitInfo.normal), 0.0f);

		if (cosThetaI > 0.0f && cosThetaY > 0.0f){

			float areaMeasureFactor = cosThetaY / r_sqr;

			float brdfPdf = payloadGI.pdf;
			float brdfPdfAreaMeasure = brdfPdf * areaMeasureFactor;

			float G = cosThetaI * cosThetaY / r_sqr;

			//these two should give the same result
			L_direct = L_i * payloadGI.f_r * G / brdfPdfAreaMeasure;
			//L_direct = L_i * payloadGI.f_r * cosThetaI / brdfPdf;
		}
	}
	return L_direct;
}
