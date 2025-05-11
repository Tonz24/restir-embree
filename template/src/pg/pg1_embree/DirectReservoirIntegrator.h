#pragma once
#include "DirectIntegrator.h"
#include "Reservoir.h"
#include "Utils.h"



class DirectReservoirIntegrator : public DirectIntegrator{
public:
	void drawGui();
	glm::vec3 calculateDirectLighting(const Scene& scene, const Ray& ray, const IntersectionResult& intersection, const RenderParams& renderParams, const glm::vec
	                                  <2,int>& pixelCoords) override;


	bool isRestir() override { return true; }

private:

	void initialRenderPass(const Scene& scene, const Ray& ray, const IntersectionResult& intersection, const RenderParams& renderParams, const glm::vec
		<2, int>& pixelCoords);


	void spatialReusePass(const Scene& scene, const Ray& ray, const IntersectionResult& intersection, const RenderParams& renderParams, const glm::vec
		<2, int>& pixelCoords);


	LightSample areaSampleLight(const Ray& ray, const IntersectionResult& intResult, const Scene& scene, const RenderParams& renderParams);
    LightSample brdfSampleLight(const Ray& ray, const IntersectionResult& intResult, const Scene& scene, const RenderParams& renderParams);


	float m_area(float pdfArea, float pdfBrdf) {
		if (pdfArea == 0.0f && pdfBrdf == 0.0f)
			return 0.0f;

		return pdfArea / (static_cast<float>(M_Area) * pdfArea + static_cast<float>(M_Brdf) * pdfBrdf);
	}

	float m_brdf(float pdfBrdf, float pdfArea) {
		if (pdfArea == 0.0f && pdfBrdf == 0.0f)
			return 0.0f;

		return pdfBrdf / (static_cast<float>(M_Area) * pdfArea + static_cast<float>(M_Brdf) * pdfBrdf);
	}

	int M_Area{1};
	int M_Brdf{1};
	int spatialReuseNeighborCount{5};
	float spatialReuseRadius{5};
	bool doReuse{false};
};