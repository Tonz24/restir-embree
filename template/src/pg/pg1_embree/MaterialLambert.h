#pragma once
#include "Distribution.h"
#include "material.h"

class MaterialLambert : public Material{
public:
	PTInfoGI evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const override;
	float getPdfForSample(const ::HitInfo& hitInfo, const ::Ray& ray, const glm::vec3& omega_i) const override;
	bool recurse() const override { return false; }
	MaterialType getType() const override { return LAMBERT; }

	BRDFEval evaluateBRDF(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i)const override;
	static glm::vec3 evalBRDF(const GBufferElement& gBufferElement, const glm::vec3& cameraPos, const glm::vec3& omega_i);
	static PTInfoGI sampleBRDF(const GBufferElement& gBufferElement, const glm::vec3& cameraPos);
};