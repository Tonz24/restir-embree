#pragma once

#include "DirectIntegrator.h"
#include "NaivePathIntegrator.h"
#include "Sampling.h"

class NEEPathIntegrator : public NaivePathIntegrator {
public:
	NEEPathIntegrator(const RenderParams& renderParams);

	void drawGui() override;

	DirectIntegrator* currentDirectIntegrator{};

	std::unique_ptr<DirectIntegrator> brdfIntegrator{};
	std::unique_ptr<DirectIntegrator> areaIntegrator{};
	std::unique_ptr<DirectIntegrator> misIntegrator{};
	std::unique_ptr<DirectIntegrator> reservoirIntegrator{};

	bool isRestir() override { return currentDirectIntegrator->isRestir(); }

protected:
	glm::vec3 integrateImpl(Ray& ray, const Scene& scene, uint32_t bounceCount, const glm::vec<2,int>& pixelCoords) override;

	virtual glm::vec3 integrateImpl2(Ray& ray, const Scene& scene, uint32_t bounceCount, VertexType lastVertexType, const glm::vec<2, int>& pixelCoords);

	bool calcDI{ true };
	bool calcGI{ true };
};