#include "stdafx.h"
#include "DirectAreaIntegrator.h"


#include "Intersection.h"
#include "Sampling.h"


glm::vec3 DirectAreaIntegrator::calculateDirectLighting(const Scene& scene, const Ray& ray, const IntersectionResult& intersection, const RenderParams& renderParams, const glm
                                                        ::vec<2,int>& pixelCoords) {

	const HitInfo& hitInfo = intersection.hitInfo;
	const Material* mat = intersection.material;

	glm::vec3 L_direct{ 0 };

	if (scene.getEmissiveCDF().isValid()) {

		//choose a triangle, return the pdf of choosing it
		const TriangleCDFSample triangleCDFSample = scene.getEmissiveCDF().getTriangle();
		//choose a point (y) on the triangle together with its normal and the pdf of choosing this point
		const TrianglePointSample trianglePoint = Sampling::sampleTriangle(triangleCDFSample.triangle);
		const float areaPdf = triangleCDFSample.pdf * trianglePoint.pdf;

		if (areaPdf == 0) return glm::vec3{ 0.0f };

		//calculate radius squared and direction from x to y, used in geometry term calculation
		glm::vec3 omega_i = trianglePoint.samplePoint - hitInfo.hitPoint;
		const float r_sqr = glm::dot(omega_i, omega_i);
		omega_i = glm::normalize(omega_i);

		if (r_sqr == 0) return glm::vec3{ 0.0f };

		const float cosThetaI = glm::max(glm::dot(omega_i, hitInfo.normal), 0.0f);
		const float cosThetaY = glm::max(glm::dot(-omega_i, trianglePoint.normal),0.0f);

		//only proceed further when it makes sense (failing the condition would evaluate direct radiance to 0) 
		if (cosThetaI > 0.0f && cosThetaY > 0.0f && !Intersection::testOcclusion(hitInfo.hitPoint, trianglePoint.samplePoint, scene, renderParams)) {

			const float G = cosThetaI * cosThetaY / r_sqr;
			//emission of the light source
			glm::vec3 L_i = triangleCDFSample.triangle.getSurface().get_material()->emission;

			// pdf of hitting point y from point x
			glm::vec3 f_r = mat->evaluateBRDF(hitInfo, ray, omega_i).f_r;

			//evaluate the rendering equation in area form
			L_direct = L_i * f_r * G / areaPdf;
		}
	}

	return L_direct;
}
