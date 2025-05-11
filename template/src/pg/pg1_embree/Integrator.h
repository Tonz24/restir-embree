#pragma once

#include "IDrawGui.h"
#include "Ray.h"
#include "RenderParams.h"
#include "Scene.h"
#include "glm/glm.hpp"

class Integrator : public IDrawGui {
public:

	Integrator(const RenderParams& renderParams) : renderParams(renderParams) {}

	glm::vec3 integrate(Ray& ray, const Scene& scene, const glm::vec<2, int>& pixelCoords)
	{
		return integrateImpl(ray, scene, 0, pixelCoords);
	}
	void drawGui() override {}

	virtual bool isRestir() { return false; }

	static bool sanitize(glm::vec3& l, bool logToConsole = true);


protected:

	virtual glm::vec3 integrateImpl(Ray& ray, const Scene& scene, uint32_t bounceCount, const glm::vec<2, int>& pixelCoords) = 0;


	const RenderParams& renderParams{};
};