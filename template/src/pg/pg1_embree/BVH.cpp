#include "stdafx.h"
#include "BVH.h"

#include <iostream>
#include <stack>

#include "Intersection.h"
#include "raytracer.h"

BVH::BVH(const std::vector<Triangle*>& triangles): triangles(triangles){

	nodes.resize(triangles.size() * 2 - 1);
	build();

	std::cout << "node count: " << nodesUsed << std::endl;
	std::cout << "leaf count: " << leafCount << std::endl;
	std::cout << "max depth: " << maxDepth<< std::endl;
}

IntersectionResult BVH::traverse(const Ray& ray) const{

	std::stack<const BVHNode*> toVisit{};
	toVisit.push(&nodes[0]);
	IntersectionResult closestIntersection{};

	while (!toVisit.empty()){
		auto node = toVisit.top();
		toVisit.pop();

		if (!Intersection::intersectAABB(ray, { node->boundsMin,node->boundsMax }))
			continue;

		if (node->isLeaf()){
			for (uint32_t i = 0; i < node->triCount; ++i){
				Triangle* tri = triangles.at(i + node->leftOrFirst);

				IntersectionResult thisIntersection = Intersection::intersectTriangle(ray, *tri);

				if (thisIntersection.hitInfo.didHit && thisIntersection.hitInfo.dst < closestIntersection.hitInfo.dst)
					closestIntersection = thisIntersection;
			}
		}
		else{
			toVisit.push(&nodes[node->leftOrFirst]);
			toVisit.push(&nodes[node->leftOrFirst + 1]);
		}
	}
	return closestIntersection;
}

bool BVH::isOccluded(const glm::vec3& from, const glm::vec3& to) const {
	const glm::vec3 lightDir = glm::normalize(to - from);
	Ray occRay{ from,lightDir};

	std::stack<const BVHNode*> toVisit{};
	toVisit.push(&nodes[0]);

	while (!toVisit.empty()) {
		auto node = toVisit.top();
		toVisit.pop();

		if (!Intersection::intersectAABB(occRay, { node->boundsMin,node->boundsMax }))
			continue;

		if (node->isLeaf()) {
			for (uint32_t i = 0; i < node->triCount; ++i) {
				const Triangle* const tri = triangles.at(i + node->leftOrFirst);
				
				if (Intersection::intersectTriangleHitOnly(occRay, *tri))
					return true;
			}
		}
		else {
			toVisit.push(&nodes[node->leftOrFirst]);
			toVisit.push(&nodes[node->leftOrFirst + 1]);
		}
	}
	return false;
}

void BVH::updateBounds(BVHNode& node) const
{
	node.boundsMin = glm::vec3{ FLT_MAX };
	node.boundsMax = glm::vec3{ -FLT_MAX };

	uint32_t first = node.leftOrFirst;
	for (int i = 0; i < node.triCount; ++i){
		Triangle* t = triangles.at(first + i);

		node.boundsMin = glm::min(node.boundsMin, t->bounds.getMin());
		node.boundsMax = glm::max(node.boundsMax, t->bounds.getMax());
	}
}

void BVH::build() {
	BVHNode& root = nodes.at(0);

	root.leftOrFirst = 0;
	root.triCount = triangles.size();
	nodesUsed = 1;

	this->updateBounds(root);
	this->subdivide(0, 1);
}

void BVH::subdivide(uint32_t nodeIndex, uint32_t currentDepth){
	BVHNode& node = nodes.at(nodeIndex);

	if (node.triCount <= maxLeavesInNode){
		leafCount++;
		return;
	}


	uint32_t axis{};
	float splitPoint{};

	if (!useSAH) {
		axis = currentDepth % 3;
		float extent = node.boundsMax[axis] - node.boundsMin[axis];
		splitPoint = node.boundsMin[axis] + extent * 0.5f;

	}
	else {
		glm::vec3 extent = node.boundsMax - node.boundsMin;
		axis = 0;

		if (extent.y > extent.x) axis = 1;
		if (extent.z > extent.y) axis = 2;

		axis = currentDepth % 3;

		const float axisScale = extent[axis] / static_cast<float>(splitPlaneCount);

		float bestCost{ FLT_MAX };
		float bestSplitPos{};

		for (int h = 1; h < splitPlaneCount; ++h){
			const float splitPos = static_cast<float>(h) * axisScale;

			const float thisCost = evaluateSAH(splitPos, axis, node);

			if (thisCost < bestCost){
				bestCost = thisCost;
				bestSplitPos = splitPos;
			}
		}
		splitPoint = bestSplitPos;

		//exit if splitting costs more
		if (bestCost >= node.calculateSAHCost()) {
			leafCount++;
			return;
		}
	}

	int i = node.leftOrFirst;
	int j = i + node.triCount - 1;

	while (i <= j) {
		if (triangles.at(i)->getCentroid()[axis] < splitPoint)
			i++;
		else
			std::swap(triangles.at(i), triangles.at(j--));
	}

	const uint32_t leftCount = i - node.leftOrFirst;
	if (leftCount == 0 || leftCount == node.triCount) 
		return;

	const uint32_t leftChildIndex = nodesUsed++;
	const uint32_t rightChildIndex = nodesUsed++;

	nodes.at(leftChildIndex).leftOrFirst = node.leftOrFirst;
	nodes.at(leftChildIndex).triCount = leftCount;

	nodes.at(rightChildIndex).leftOrFirst = i;
	nodes.at(rightChildIndex).triCount = node.triCount - leftCount;

	node.leftOrFirst = leftChildIndex;
	node.triCount = 0;

	updateBounds(nodes.at(leftChildIndex));
	updateBounds(nodes.at(rightChildIndex));

	maxDepth = std::max(currentDepth, maxDepth);

	subdivide(leftChildIndex, currentDepth + 1);
	subdivide(rightChildIndex, currentDepth + 1);
}

float BVH::evaluateSAH(float splitPos, uint32_t axis, const BVHNode& node) const{

	uint32_t leftCount{}, rightCount{};
	AABB leftBounds{}, rightBounds{};

	for (uint32_t i = node.leftOrFirst; i < node.leftOrFirst + node.triCount; ++i){
		const Triangle& tri = *triangles.at(i);

		if (tri.getCentroid()[axis] < splitPos){
			leftCount++;
			leftBounds.grow(tri[0].position);
			leftBounds.grow(tri[1].position);
			leftBounds.grow(tri[2].position);
		}

		else{
			rightCount++;
			rightBounds.grow(tri[0].position);
			rightBounds.grow(tri[1].position);
			rightBounds.grow(tri[2].position);
		}
	}

	const float cost = leftBounds.getSurfaceArea() * static_cast<float>(leftCount) + rightBounds.getSurfaceArea() * static_cast<float>(rightCount);
	return cost > 0 ? cost : FLT_MAX;
}
