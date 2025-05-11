#pragma once
#include "NEEPathIntegrator.h"


class HardMisIntegrator : public NEEPathIntegrator {
public:
	HardMisIntegrator(const RenderParams& renderParams, const RayHitOffsets& offsets) : NEEPathIntegrator(renderParams, offsets){}
protected:
	glm::vec3 integrateImpl2(Ray& ray, const Scene& scene, uint32_t bounceCount, VertexType lastVertexType) override;

public:
	void drawGui() override {}
};

