#include "stdafx.h"
#include "Intersection.h"
#include "surface.h"

IntersectionResult Intersection::intersectTriangle(const Ray& ray, const Triangle& triangle)
{
	HitInfo hit{};
	Material* material{};

	glm::vec3 edgeAB = triangle[1].position - triangle[0].position;
	glm::vec3 edgeAC = triangle[2].position - triangle[0].position;
	glm::vec3 normalVec = cross(edgeAB, edgeAC);
	glm::vec3 ao = ray.getOrg() - triangle[0].position;
	glm::vec3 dao = cross(ao, ray.getDir());

	float det = -glm::dot(ray.getDir(), normalVec);
	float invDet = 1.0f / det;

	float dst = glm::dot(ao, normalVec) * invDet;
	float u = glm::dot(edgeAC, dao) * invDet;
	float v = -glm::dot(edgeAB, dao) * invDet;
	float w = 1.0f - u - v;

	hit.normal = glm::normalize(triangle[0].normal * (1.0f - u - v) + triangle[1].normal * u + triangle[2].normal * v);
	hit.uv = triangle[0].texture_coords[0] * w + u * triangle[1].texture_coords[0] + v * triangle[2].texture_coords[0];

	if (glm::dot(ray.getDir(), normalVec) > 0.0) {
		hit.fromInside = true;
		hit.normal *= -1.0f;
	}

	material = triangle.getSurface().get_material();
	hit.hitPoint = ray.getOrg() + ray.getDir() * dst;
	hit.dst = dst;
	hit.didHit = glm::abs(det) >= 0.0001f && dst > 0.0f && u >= 0.0f && v >= 0.0f && u + v <= 1.0f;

	return {hit,material};
}

bool Intersection::intersectTriangleHitOnly(const Ray& ray, const Triangle& triangle){

	glm::vec3 edgeAB = triangle[1].position - triangle[0].position;
	glm::vec3 edgeAC = triangle[2].position - triangle[0].position;
	glm::vec3 normalVec = cross(edgeAB, edgeAC);
	glm::vec3 ao = ray.getOrg() - triangle[0].position;
	glm::vec3 dao = cross(ao, ray.getDir());

	float det = -glm::dot(ray.getDir(), normalVec);
	float invDet = 1.0f / det;

	float dst = glm::dot(ao, normalVec) * invDet;
	float u = glm::dot(edgeAC, dao) * invDet;
	float v = -glm::dot(edgeAB, dao) * invDet;
	float w = 1.0f - u - v;

	return glm::abs(det) >= 0.0001f && dst > 0.0f && u >= 0.0f && v >= 0.0f && u + v <= 1.0f;;
}

bool Intersection::intersectAABB(const Ray& ray, const AABB& aabb) {
	const glm::vec3 tMin = (aabb.getMin() - ray.getOrg()) * ray.getRcpDir();
	const glm::vec3 tMax = (aabb.getMax() - ray.getOrg()) * ray.getRcpDir();

	const glm::vec3 t1 = glm::min(tMin, tMax);
	const glm::vec3 t2 = glm::max(tMin, tMax);

	const float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
	const float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);

	return tNear <= tFar;
}
