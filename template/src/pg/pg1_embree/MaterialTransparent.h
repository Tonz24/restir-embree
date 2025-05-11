#pragma once
#include "material.h"

class MaterialTransparent : public Material{
public:
	PTInfoGI evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const override;
	bool recurse() const override{
		return true;
	}

	MaterialType getType() const override{
		return DIELECTRIC_TRANSPARENT;
	}
};