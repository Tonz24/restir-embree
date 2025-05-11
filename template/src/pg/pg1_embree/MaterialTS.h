#pragma once
#include "material.h"
class MaterialTS : public Material {
public:
	PTInfoGI evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const override;
	bool recurse() const override { return false; }
	MaterialType getType() const override { return LAMBERT; }

	BRDFEval evaluateBRDF(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i)const override;

private:
	float evalGGX(float nDotM) const;
	float evalGSmith(float nDotO, float nDotI) const;
	float evalGAux(float dot) const;
	float evalSchlick(float iorFrom, float iorTo, float cosH) const;
};

