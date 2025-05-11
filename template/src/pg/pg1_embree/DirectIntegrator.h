#pragma once
#include "IDrawGui.h"
#include "IntersectionResult.h"
#include "RenderParams.h"
#include "Scene.h"
#include "glm/glm.hpp"

class DirectIntegrator : public IDrawGui{
public:
	DirectIntegrator() = default;
	~DirectIntegrator() override = default;

	//for a known intersection, calculate direct lighting contribution
	virtual glm::vec3 calculateDirectLighting(const Scene& scene, const Ray& ray, const IntersectionResult& intersection, const RenderParams& offsets, const glm::vec
	                                          <2, int>& pixelCoords) = 0;
	virtual void drawGui() override {}


	virtual bool isRestir() { return false; }
};