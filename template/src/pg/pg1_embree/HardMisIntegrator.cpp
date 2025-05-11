#include "stdafx.h"
#include "HardMisIntegrator.h"

#include "Intersection.h"
#include "utils.h"

glm::vec3 HardMisIntegrator::integrateImpl2(Ray& ray, const Scene& scene, uint32_t bounceCount,
                                            VertexType lastVertexType)
{
	if (bounceCount > renderParams.maxBounceCount)
		return glm::vec3{ 0 };


	//see if the ray hit anything
	auto intResult = Intersection::intersectEmbree(scene, ray, offsets, false);
	const HitInfo& hitInfo = intResult.hitInfo;

	if (hitInfo.didHit) {

		//use the intersection as direct brdf sample, terminate path
		if (intResult.material->isEmitter()) {
			glm::vec3 L_direct_brdf{};
			glm::vec3 L_i = intResult.material->emission;


			glm::vec3 lightDir = hitInfo.hitPoint - ray.getOrg();
			float r_sqr = glm::dot(lightDir, lightDir);
			lightDir = glm::normalize(lightDir);

			float cosThetaI = glm::max(glm::dot(lightDir, hitInfo.normal), 0.0f);
			float cosThetaY = glm::max(glm::dot(-lightDir, hitInfo.normal), 0.0f);

			float areaMeasureFactor = cosThetaY / r_sqr;
			
			//float brdfPdfAreaMeasure = brdfPdf * areaMeasureFactor;

			return L_direct_brdf;
		}


		//direct light sample
		glm::vec3 L_direct_light_sample{ 0 };
		if (scene.getEmissiveCDF().isValid()) {

			//first sample an emissive triangle
			//pick a triangle relative to the surface area CDF
			auto triangleCDFSample = scene.getEmissiveCDF().getTriangle();
			//pick a point on that selected triangle
			auto trianglePoint = Sampling::sampleTriangle(triangleCDFSample.triangle);
			float lightPdf = triangleCDFSample.pdf * trianglePoint.pdf;
			float pdfAsIfBrdf{ 0 };

			assert(lightPdf > 0);

			//calculate radius squared and direction from x to y, used in geometry term calculation
			glm::vec3 lightDir = trianglePoint.samplePoint - hitInfo.hitPoint;
			float r_sqr = glm::dot(lightDir, lightDir);
			lightDir = glm::normalize(lightDir);

			assert(r_sqr > 0);

			//X is original intersection hit point
			float cosThetaX = glm::max(glm::dot(lightDir, hitInfo.normal), 0.0f); //cosine between hitpoint normal and light direction
			//Y is light sample point
			float cosThetaY = glm::max(glm::dot(-lightDir, trianglePoint.normal), 0.0f); //cosine between light sample normal and negative light direction

			//float angleMeasureFactor = r_sqr / cosThetaY;
			float areaMeasureFactor = cosThetaY / r_sqr;

			pdfAsIfBrdf = intResult.material->getPdfForSample(hitInfo, ray, lightDir) * areaMeasureFactor;
			glm::vec3 L_i = triangleCDFSample.triangle.getSurface().get_material()->emission;

			float G = cosThetaX * cosThetaY / r_sqr;
			glm::vec3 f_r = intResult.material->evaluateBRDF(hitInfo, ray, lightDir).f_r;

			float misWeight = Utils::powerHeuristic(lightPdf, pdfAsIfBrdf);

			L_direct_light_sample = misWeight * L_i * f_r * G / lightPdf;

			//add the angular and area parts together
			return L_direct_light_sample;
		}

		//GI calculation
		auto payloadGI = intResult.material->evaluateLightingGI(hitInfo, ray);

		//reuse ray for tracing next bounce
		ray.setOrg(hitInfo.hitPoint + hitInfo.normal * offsets.normalOffset);
		ray.setDir(payloadGI.omega_i);
		ray.getRTCRay().tnear = FLT_MIN + offsets.tnearOffset;
		ray.getRTCRay().tfar = FLT_MAX;

		//evaluate the rendering equation in angular form (recursive step)
		glm::vec3 renderingEq = integrateImpl2(ray, scene, bounceCount + 1, payloadGI.vertexType);

		float cosThetaI = glm::max(glm::dot(payloadGI.omega_i, hitInfo.normal), 0.0f);
		glm::vec3 L_i_indirect = renderingEq * payloadGI.f_r * cosThetaI / payloadGI.pdf;


		return L_i_indirect + L_direct_light_sample;
	}
	return renderParams.useSkybox ? scene.getSkybox().getTexel(ray.getDir()) : renderParams.bgColor;
}
