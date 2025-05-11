#pragma once
#include "AABB.h"

struct BVHNode {
	glm::vec3 boundsMin{FLT_MAX};
	uint32_t leftOrFirst;
	glm::vec3 boundsMax{-FLT_MAX};
	uint32_t triCount;

	bool isLeaf() const{
		return triCount > 0;
	}

	float calculateSAHCost() const {
		glm::vec3 extent = boundsMax - boundsMin;
		return (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z) * 2.0f * triCount;
	}
};
