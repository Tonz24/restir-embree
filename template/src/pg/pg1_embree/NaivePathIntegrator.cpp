#include "stdafx.h"
#include "NaivePathIntegrator.h"

#include <iostream>


#include "Intersection.h"
#include "Sampling.h"
#include "Utils.h"

void NaivePathIntegrator::drawGui(){
	ImGui::Checkbox("Apply Russian Roulette", &RR);
}

glm::vec3 NaivePathIntegrator::integrateImpl(Ray& ray, const Scene& scene,
                                             uint32_t bounceCount, const glm::vec<2,int>& pixelCoords)
{
	if (bounceCount > renderParams.maxBounceCount)
		return glm::vec3{ 0 };

	auto intResult = Intersection::intersectEmbree(scene, ray);;
	const HitInfo& hitInfo = intResult.hitInfo;

	if (hitInfo.didHit) {

		float maxDiff = Utils::maxComponent(intResult.material->diffuse);
		float maxSpec = Utils::maxComponent(intResult.material->specular);

		float maxThroughput = glm::max(maxDiff, maxSpec);

		if (bounceCount > 5 && RR) {
			if (maxThroughput <= Utils::getRandomValue(0.0f, 1.0f)) {
				return glm::vec3{ 0 };
			}
		}

		if (intResult.material->isEmitter()) 
			return intResult.material->emission;

		//GI calculation
		auto payloadGI = intResult.material->evaluateLightingGI(hitInfo, ray);

		glm::vec3 L_o{ 0 };

		float cosThetaI = glm::max(glm::dot(payloadGI.omega_i, hitInfo.normal),0.0f);

		ray.setOrg(hitInfo.hitPoint + renderParams.normalOffset * hitInfo.normal);
		ray.setDir(payloadGI.omega_i);
		ray.getRTCRay().tnear = FLT_MIN + renderParams.tnearOffset;
		ray.getRTCRay().tfar = FLT_MAX;

		//evaluate the rendering equation in angular form
		glm::vec3 L_i = integrateImpl(ray, scene, bounceCount + 1, pixelCoords);

		L_o = L_i * payloadGI.f_r * cosThetaI / (payloadGI.pdf * ((bounceCount > 5 && RR) ? maxThroughput : 1.0f));

		sanitize(L_o);

		return L_o;
	}
	return renderParams.useSkybox ? scene.getSkybox().getTexel(ray.getDir()) : renderParams.bgColor;
}
