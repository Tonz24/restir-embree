#include "stdafx.h"
#include "MaterialTransparent.h"
#include "Utils.h"


PTInfoGI MaterialTransparent::evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const{
	const glm::vec3 reflectionDir = glm::reflect(ray.getDir(), hitInfo.normal);
	const glm::vec3 refractionDir = glm::refract(ray.getDir(), hitInfo.normal, hitInfo.fromInside ? this->ior : 1.0f / this->ior);
	const float thetaI = glm::abs(glm::dot(reflectionDir, hitInfo.normal));

	const float reflCoeff = Utils::schlickApprox(ray.getDir(), hitInfo.normal,
		hitInfo.fromInside ? this->ior : 1.0f,
		hitInfo.fromInside ? 1.0f : this->ior);

	glm::vec3 omega_i{};
	glm::vec3 f_r{this->specular / thetaI};
	float pdf{};
	VertexType vertexType{INVALID_VERTEX};


	if (Utils::getRandomValue() < reflCoeff) { //specular reflection
		omega_i = reflectionDir;
		pdf = reflCoeff;
		f_r *= reflCoeff;
		vertexType = SPECULAR_VERTEX;
	}
	else { //refract
		omega_i = refractionDir;
		pdf = 1.0f - reflCoeff;
		vertexType = REFRACTIVE_VERTEX;
		f_r *= 1.0f - reflCoeff;
		if (hitInfo.fromInside) //attenuate using Beer's law
				f_r *= glm::exp(-this->attenuation * hitInfo.dst);
	}

	return { omega_i,f_r,pdf,vertexType};
}
