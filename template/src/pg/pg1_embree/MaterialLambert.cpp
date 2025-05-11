#include "stdafx.h"
#include "MaterialLambert.h"

#include "raytracer.h"
#include "Sampling.h"
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtc/constants.hpp"


PTInfoGI MaterialLambert::evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const {
	glm::vec3 albedo = getDiffuseColor(hitInfo.uv);

	const auto omega_i = CosineWeightedDistribution::sample(hitInfo.normal);
	const auto pdf = CosineWeightedDistribution::getPdf(hitInfo.normal,omega_i);
	const auto f_r = albedo / glm::pi<float>();

	return { omega_i, f_r, pdf, DIFFUSE_VERTEX};
}

float MaterialLambert::getPdfForSample(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const
{
	return CosineWeightedDistribution::getPdf(hitInfo.normal, omega_i);
}

BRDFEval MaterialLambert::evaluateBRDF(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const {
	glm::vec3 albedo = getDiffuseColor(hitInfo.uv);

	const glm::vec3 f_r = albedo * glm::one_over_pi<float>();

	return {f_r};
}

glm::vec3 MaterialLambert::evalBRDF(const GBufferElement& gBufferElement, const glm::vec3& cameraPos,
	const glm::vec3& omega_i) {

	glm::vec3 albedo = gBufferElement.diffuseColor;

	const glm::vec3 f_r = albedo * glm::one_over_pi<float>();

	return f_r;
}

PTInfoGI MaterialLambert::sampleBRDF(const GBufferElement& gBufferElement, const glm::vec3& cameraPos) {

	glm::vec3 diffuse = gBufferElement.diffuseColor;
	glm::vec3 specular = gBufferElement.specularColor;

	const auto omega_i = CosineWeightedDistribution::sample(gBufferElement.worldSpaceNormal);
	const auto pdf = CosineWeightedDistribution::getPdf(gBufferElement.worldSpaceNormal, omega_i);
	const auto f_r = diffuse / glm::pi<float>();

	return { omega_i, f_r, pdf, DIFFUSE_VERTEX };
}
