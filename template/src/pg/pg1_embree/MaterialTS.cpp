#include "stdafx.h"
#include "MaterialTS.h"

#include "Utils.h"
#include "glm/gtc/constants.hpp"

BRDFEval MaterialTS::evaluateBRDF(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const {
	const glm::vec3 omega_o = -ray.getDir();
	const glm::vec3 omega_m = (omega_o + omega_i) / 2.0f;
	

	const glm::vec3 baseColor = getDiffuseColor(hitInfo.uv);

	///////////////////////////////////////////////////////////////////
	//diffuse part of BRDF
	glm::vec3 f_r = baseColor * glm::one_over_pi<float>();
	///////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////
	//specular part of BRDF
	const float mDotI = glm::max(glm::dot(omega_i, omega_m), 0.0f);
	const float mDotO = glm::max(glm::dot(omega_o, omega_m), 0.0f);
	const float nDotM = glm::max(glm::dot(omega_m, hitInfo.normal), 0.0f);

	const float G = evalGSmith(mDotO, mDotI);
	const float D = evalGGX(nDotM);
	const float F = evalSchlick(1.0, ior, mDotI);

	float denom = mDotI * mDotO;
	f_r += glm::vec3{ 0.25f * D * F * G / denom };
	///////////////////////////////////////////////////////////////////

	return {f_r};
}

float MaterialTS::evalGGX(float nDotM) const {
	const float alpha = roughness * roughness;
	if (alpha == 1) return glm::one_over_pi<float>();

	const float aSqr = alpha * alpha;

	float denomInner = (aSqr - 1.0f) * (nDotM * nDotM) + 1.0f;
	denomInner *= denomInner;

	return glm::one_over_pi<float>() * aSqr / denomInner;
}

float MaterialTS::evalGSmith(float nDotO, float nDotI) const {
	return 1.0f / (1.0f + evalGAux(nDotO) + evalGAux(nDotI));
}

float MaterialTS::evalGAux(float dot) const {
	const float alpha = roughness * roughness;
	const float aSqr = alpha * alpha;
	
	const float innerFrac = 1.0f / (dot * dot) - 1.0f;
	const float root = glm::sqrt(1.0f + aSqr * innerFrac);

	return (root - 1.0f) / 2.0f;
}

float MaterialTS::evalSchlick(float iorFrom, float iorTo, float cosH) const{
	float F0 = (iorFrom - iorTo) / (iorFrom + iorTo);
	F0 *= F0;
	float cosTerm = 1.0f - cosH;
	cosTerm = cosTerm * cosTerm * cosTerm * cosTerm * cosTerm;

	return F0 + (1.0f - F0) * cosTerm;
}