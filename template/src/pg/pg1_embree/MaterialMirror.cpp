#include "stdafx.h"
#include "MaterialMirror.h"

PTInfoGI MaterialMirror::evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const {
	const auto omega_i = glm::reflect(ray.getDir(), hitInfo.normal);
	const auto pdf = 1.0f; //delta functions cancel each other out
	glm::vec3 f_r{0};

	const float thetaI = glm::max(glm::dot(omega_i, hitInfo.normal),0.0f);
	f_r = this->specular / thetaI; //dot product cancels out during rendering equation calculation

	return { omega_i, f_r, pdf, MIRROR_VERTEX};
}