#pragma once
#include "DirectIntegrator.h"

class DirectBRDFIntegrator : public DirectIntegrator {
public:
	glm::vec3 calculateDirectLighting(const Scene& scene, const Ray& ray, const IntersectionResult& intersection,
	                                  const RenderParams& offsets, const glm::vec<2,int>& pixelCoords) override;
};