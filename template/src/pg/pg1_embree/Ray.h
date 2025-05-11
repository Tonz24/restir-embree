#pragma once
#include "glm/glm.hpp"

class Ray{
public:

	Ray() = default;
	Ray(const glm::vec3& org, const glm::vec3& dir, float tnear = FLT_MIN + 0.01f, float tfar = FLT_MAX);

	Ray(const Ray& ray_);

	const glm::vec3& getOrg() const;
	const glm::vec3& getDir() const;
	const glm::vec3& getRcpDir() const;
	const RTCRay& getRTCRay() const;
	RTCRay& getRTCRay();
	glm::vec3 getHitPoint() const;


	void setRTCRay(const RTCRay& ray);
	void setOrg(const glm::vec3& newOrg);
	void setDir(const glm::vec3& newDir);

private:
	//Embree ray structure
	RTCRay ray_embree{};

	glm::vec3 rcpDir{};
};