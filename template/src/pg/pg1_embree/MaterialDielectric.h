#pragma once
#include "MaterialPhong.h"


class MaterialDielectric : public MaterialPhong {
public:
	PTInfoGI evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const override;
	BRDFEval evaluateBRDF(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const override;
	float getPdfForSample(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const override;

	bool recurse() const override{return true;}
	MaterialType getType() const override{return DIELECTRIC;}
};

