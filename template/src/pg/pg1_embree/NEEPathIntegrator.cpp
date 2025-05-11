#include "stdafx.h"
#include "NEEPathIntegrator.h"

#include "Intersection.h"
#include "glm/gtx/string_cast.hpp"

#include <iostream>

#include "DirectAreaIntegrator.h"
#include "DirectBRDFIntegrator.h"
#include "DirectMISIntegrator.h"
#include "DirectReservoirIntegrator.h"

NEEPathIntegrator::NEEPathIntegrator(const RenderParams& renderParams): NaivePathIntegrator(renderParams)
{

	brdfIntegrator = std::make_unique<DirectBRDFIntegrator>();
	areaIntegrator = std::make_unique<DirectAreaIntegrator>();
	misIntegrator = std::make_unique<DirectMISIntegrator>();
	reservoirIntegrator = std::make_unique<DirectReservoirIntegrator>();

	currentDirectIntegrator = areaIntegrator.get();
}

void NEEPathIntegrator::drawGui()
{
	NaivePathIntegrator::drawGui();
	ImGui::Checkbox("Calculate DI", &calcDI);
	ImGui::Checkbox("Calculate GI", &calcGI);

	static const char* items[]{ "Light Source Sampler", "BRDF Sampler", "MIS Sampler", "RIS MIS Sampler"};
	static int integratorIndex = 0;

	if (ImGui::Combo("Direct Lighting Integrator", &integratorIndex, items, IM_ARRAYSIZE(items))) {
		switch (integratorIndex) {

		case 1:
			currentDirectIntegrator = brdfIntegrator.get();
			break;
		case 2:
			currentDirectIntegrator = misIntegrator.get();
			break;
		case 3:
			currentDirectIntegrator = reservoirIntegrator.get();
			break;
		case 0:
		default:
			currentDirectIntegrator = areaIntegrator.get();
			break;
		}
	}
	currentDirectIntegrator->drawGui();
}

glm::vec3 NEEPathIntegrator::integrateImpl(Ray& ray, const Scene& scene, uint32_t bounceCount, const glm::vec<2,int>& pixelCoords) {
	return integrateImpl2(ray, scene, bounceCount, CAMERA_VERTEX, pixelCoords);
}

/*
 *	NEE: pokud indirect paprsek taky trefi svetlo, tak nevracet L_e (vracet 0), jinak by prispevek svetla byl zapocitan dvakrat
 *
 *	specialni pripady:
 *		-	pokud indirect paprsek pri prvnim odrazu trefi svetlo, tak hned vratit L_e
 *		-	zrcadlo pro direct paprsek vzdycky vrati f_r = 0, jelikoz BRDF v tomto smeru != smer odrazu pro ()
 *			-	uchovavat informaci, zda predchozi hit byl do spekularniho materialu (mozna se na to vykaslat)
 * geometry term: y je nahodny bod na trojuhelniku
 *
 *	stejne velke trojuhelniky: pdf navazit inverzi pravdepodobnosti vyberu prave tohoto trojuhelniku (1 / pocet)
 * znormalizovat plochy trojuhelniku pred vytvorenim
 * pdf -> cdf rozdil vybraneho s dalsim mensim
 */
glm::vec3 NEEPathIntegrator::integrateImpl2(Ray& ray, const Scene& scene,
                                            uint32_t bounceCount, VertexType lastVertexType, const glm::vec<2, int>& pixelCoords)
{
	if (bounceCount > renderParams.maxBounceCount)
		return glm::vec3{ 0 };

	auto intResult = Intersection::intersectEmbree(scene, ray);
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

		if (intResult.material->isEmitter()) {
			if (lastVertexType == CAMERA_VERTEX || lastVertexType == MIRROR_VERTEX)
				return intResult.material->emission;
			return glm::vec3{ 0 };
		}

		glm::vec3 L_i_direct{0};
		if (calcDI)
			L_i_direct = currentDirectIntegrator->calculateDirectLighting(scene,ray,intResult,renderParams, pixelCoords);
		
		sanitize(L_i_direct);

		glm::vec3 L_i_indirect{ 0 };
		if (calcGI) {
			//GI calculation
			auto payloadGI = intResult.material->evaluateLightingGI(hitInfo, ray);

			//reuse ray for tracing next bounce
			ray.setOrg(hitInfo.hitPoint + hitInfo.normal * renderParams.normalOffset);
			ray.setDir(payloadGI.omega_i);
			ray.getRTCRay().tnear = FLT_MIN + renderParams.tnearOffset;
			ray.getRTCRay().tfar = FLT_MAX;

			//evaluate the rendering equation in angular form (recursive step)
			glm::vec3 renderingEq = integrateImpl2(ray, scene, bounceCount + 1, payloadGI.vertexType, pixelCoords);

			float cosThetaI = glm::abs(glm::dot(payloadGI.omega_i, hitInfo.normal));

			if (cosThetaI < 0)
				int h = 1;

			L_i_indirect = renderingEq * payloadGI.f_r * cosThetaI / (payloadGI.pdf * ((bounceCount > 5 && RR) ? maxThroughput : 1.0f));
		}
		sanitize(L_i_indirect);

		//add the angular and area parts together
		return L_i_indirect + L_i_direct;
	}
	return renderParams.useSkybox ? scene.getSkybox().getTexel(ray.getDir()) : renderParams.bgColor;
}