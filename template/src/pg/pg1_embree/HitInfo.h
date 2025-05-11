#pragma once
#include "glm/glm.hpp"

struct HitInfo {
	glm::vec3 normal{};
	glm::vec3 tangent{};
	glm::vec2 uv{};

	bool didHit{ false };
	bool fromInside{ false };

	glm::vec3 hitPoint{};
	float dst{ FLT_MAX };

	uint64_t visibilityMask{};

	uint32_t hitTriId{};

	float getOcclusionFactor(int lightIndex) const{
		const uint64_t occFactor = visibilityMask >> lightIndex & 1;
		return static_cast<float>(occFactor);
	}
};