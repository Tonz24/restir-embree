#pragma once

#include "Integrator.h"

//no NEE
class NaivePathIntegrator : public Integrator {
public:
	NaivePathIntegrator(const RenderParams& renderParams)
		: Integrator(renderParams)
	{
	}

	void drawGui() override;

protected:
	glm::vec3 integrateImpl(Ray& ray, const Scene& scene, uint32_t bounceCount, const glm::vec<2,int>& pixelCoords) override;

	bool RR{true};
};