#pragma once
#include "DirectIntegrator.h"
#include "IntersectionResult.h"



class DirectMISIntegrator : public DirectIntegrator {
public:
	glm::vec3 calculateDirectLighting(const Scene& scene, const Ray& ray, const IntersectionResult& intersection, const RenderParams& offsets, const glm::vec
	                                  <2,int>& pixelCoords) override;

	void drawGui() override;
	static float powerHeuristic(float pdf, float pdfOther);

private:

	glm::vec3 evaluateLightSample(const Scene& scene, const IntersectionResult& intersection, const Ray& ray, const RenderParams& offsets) const;
	glm::vec3 evaluateBRDFSample(const Scene& scene, const IntersectionResult& intersection, const Ray& ray, const RenderParams& offsets) const;
	//glm::vec3 evaluateEnvMapSample(const Scene& scene, const IntersectionResult& intersection, const Ray& ray);

	bool sampleBRDF{ true };
	bool sampleLightSources{ true };
	//TODO: Implement environment map importance sampling
	//bool sampleEnvMap{ false };
	bool showWeights{ false };
};

