#pragma once

#include "utils.h"
#include "Ray.h"

struct LightSample {

	glm::vec3 samplePoint{-FLT_MAX}, sampleNormal{ -FLT_MAX};
	glm::vec3 L_i{ -FLT_MAX };

	bool isValid() const{
		bool pointOk = samplePoint.x != -FLT_MAX && samplePoint.y != -FLT_MAX  && samplePoint.z != -FLT_MAX;
		bool normalOk = sampleNormal.x != -FLT_MAX && sampleNormal.y != -FLT_MAX  && sampleNormal.z != -FLT_MAX;
		bool L_iOk = L_i.x > 0 || L_i.y > 0 || L_i.z > 0;

		return pointOk && normalOk && L_iOk;
	}
};

struct InitialCandidateSample{
	LightSample sample{};

	float W{0};
	float misWeight{};
};

struct Reservoir {
	LightSample bestSample{};
	float w_sum{ 0 };
	float W{0};


	bool addSample(const LightSample& sample, float w, int confidence) {

		//samplesProcessed += 1;
		w_sum += w;
		this->confidence += confidence;

		if (w == 0 && w_sum == 0)
			return false;

		if (Utils::getRandomValue(0, 1) < w / w_sum){
			bestSample = sample;
			return true;
		}
		return false;
	}

	bool hasSample() const{
		//should never be less than 0 but who knows
		return w_sum > 0.0f;
	}

	void capConfidence(int confidenceCap){
		confidence = glm::min(confidence, confidenceCap);
	}

	int confidence{ 0 };

};