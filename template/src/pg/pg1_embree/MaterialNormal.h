#pragma once
#include "material.h"

class MaterialNormal : public Material {
public:
	MaterialType getType() const override { return NORMAL; }
};