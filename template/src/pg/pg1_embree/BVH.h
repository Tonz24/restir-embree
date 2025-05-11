#pragma once

#include "BVHNode.h"
#include "IntersectionResult.h"
#include "Ray.h"
#include "triangle.h"

struct Bin{
	AABB bounds{};
	uint32_t triCount;
};



class BVH{
public:

	BVH() = default;

	BVH(const std::vector<Triangle*>& triangles);

	IntersectionResult traverse(const Ray& ray) const;
	bool isOccluded(const glm::vec3& from, const glm::vec3& to) const;

	std::vector <Triangle*> triangles{};

private:
	void updateBounds(BVHNode& node) const;
	void build();
	void subdivide(uint32_t nodeIndex, uint32_t currentDepth);
	float evaluateSAH(float splitPos, uint32_t axis, const BVHNode& node) const;


	int maxLeavesInNode{1};
	int splitPlaneCount{16};
	bool useSAH{ false };

	std::vector<BVHNode> nodes{};

	uint32_t nodesUsed{ 0 };
	int leafCount{ 0 };
	uint32_t maxDepth{ 0 };
};