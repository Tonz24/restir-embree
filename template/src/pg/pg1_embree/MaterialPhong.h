#pragma once
#include "Distribution.h"
#include "material.h"

class MaterialPhong : public Material{
public:
	PTInfoGI evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const override;
	BRDFEval evaluateBRDF(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const override;
	float getPdfForSample(const ::HitInfo& hitInfo, const ::Ray& ray, const glm::vec3& omega_i) const override;

	static glm::vec3 evalBRDF(const GBufferElement& gBufferElement, const glm::vec3& cameraPos, const glm::vec3& omega_i);
	static float evalPdf(const GBufferElement& gBufferElement, const glm::vec3& cameraPos, const glm::vec3& omega_i);
	static PTInfoGI sampleBRDF(const GBufferElement& gBufferElement, const glm::vec3& cameraPos);

	bool recurse() const override { return false; }
	MaterialType getType() const override { return PHONG; }

protected:
	static float calc_I_M(float nDotV, float n);

private:
	static float gamma_quot(float a, float b);
	static float ibeta(float x, float a, float b);
};