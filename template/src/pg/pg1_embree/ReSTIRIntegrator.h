#pragma once

#include "camera.h"
#include "DirectAreaIntegrator.h"
#include "DirectAreaIntegrator.h"
#include "DirectAreaIntegrator.h"
#include "DirectAreaIntegrator.h"
#include "DirectAreaIntegrator.h"
#include "DirectAreaIntegrator.h"
#include "Distribution.h"
#include "Integrator.h"
#include "MaterialDielectric.h"
#include "MaterialLambert.h"
#include "MaterialPhong.h"
#include "Reservoir.h"

class ReSTIRIntegrator{
private:
	enum SpatialWeightCalculation {
		CONSTANT,
		CONSTANT_DEBIAS_CONTRIB,
		CONSTANT_DEBIAS_Z_TERM,
		BALANCE_HEURISTIC,
		PAIRWISE_MIS
	};

public:

	static InitialCandidateSample areaSampleLight(const Scene& scene, const glm::vec<2,int>& pixelCoord);
	static InitialCandidateSample brdfSampleLight(const Scene& scene, const glm::vec<2, int>& pixelCoord);

	static auto getMaterialBRDFEvalFunc(MaterialType materialType){

		if (materialType == PHONG)
			return MaterialPhong::evalBRDF;
		if (materialType == LAMBERT)
			return MaterialLambert::evalBRDF;
		if (materialType == DIELECTRIC)
			return MaterialDielectric::evalBRDF;
		return  MaterialLambert::evalBRDF;
	}

	static auto getMaterialSampleFunc(MaterialType materialType) {

		if (materialType == PHONG)
			return MaterialPhong::sampleBRDF;
		if (materialType == LAMBERT)
			return MaterialLambert::sampleBRDF;
		if (materialType == DIELECTRIC)
			return MaterialDielectric::sampleBRDF;
		return  MaterialPhong::sampleBRDF;
	}

	static auto getMaterialPDFEvalFunc(MaterialType materialType) {

		if (materialType == PHONG)
			return MaterialPhong::evalPdf;
		return MaterialPhong::evalPdf;
	}


	static float m_area(float pdfArea, float pdfBrdf) {
		if (pdfArea == 0.0f && pdfBrdf == 0.0f)
			return 0.0f;

		return pdfArea / (static_cast<float>(M_Area) * pdfArea + static_cast<float>(M_Brdf) * pdfBrdf);
	}

	static float m_brdf(float pdfBrdf, float pdfArea) {
		if (pdfArea == 0.0f && pdfBrdf == 0.0f)
			return 0.0f;

		return pdfBrdf / (static_cast<float>(M_Area) * pdfArea + static_cast<float>(M_Brdf) * pdfBrdf);
	}

	static float evaluatePHat(const LightSample& sample, const Scene& scene, const glm::vec3& cameraPos, const
	                          GBufferElement& gBufferElem, bool testVisibility);
	static glm::vec3 evaluateF(const LightSample& sample, const Scene& scene, const glm::vec3& cameraPos, const
	                           GBufferElement& gBufferElem, bool testVisibility);

	static void gBufferFillPass(const Scene& scene, const glm::vec<2, int>& pixelCoords, const Camera& camera);
	static void initialRenderPass(const Scene& scene, const glm::vec<2, int>& pixelCoords);
	static void visibilityPass(const Scene& scene, const glm::vec<2, int>& pixelCoords);
	static void spatialReusePass(const glm::vec <2, int>& pixelCoords, const Scene& scene);
	static void temporalReusePass(const glm::vec <2, int>& pixelCoords, const Scene& scene, const Camera& camera);


	static bool testReprojection();


	static int confidenceCap;
	static void gui2();

	static RenderParams renderParams;

	static SpatialWeightCalculation spatialWeightCalc;
	static constexpr glm::vec<2, int> INVALID_REPROJECTION{-1,-1};

	static int spatialPassCount;
	static bool doUnbiasedTemporalMIS;
	static bool weighLastFrame;
	static bool doVisibilityPass;
	static bool rejectDissimilarNeighbors;
	static float minNormalSimilarity;
	static float maxDepthDifference;

	static int M_Area;
	static int M_Brdf;
	static int spatialReuseNeighborCount;
	static float spatialReuseRadius;
	static bool doSpatialReuse;
	static bool doTemporalReuse;
	static bool debugReprojection;

private:
	static glm::vec<2, int> reprojectBackward(const glm::vec3& wsPos, const glm::vec<2,int>& pixelCoords);
	static glm::vec<2, int> reprojectForward(const glm::vec3& wsPos, const glm::vec<2,int>& pixelCoords);


};