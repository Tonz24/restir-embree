#include "stdafx.h"
#include "Ray.h"

Ray::Ray(const glm::vec3& org, const glm::vec3& dir, float tnear, float tfar){
	setOrg(org);
	setDir(dir);

	ray_embree.tnear = tnear;
	ray_embree.tfar = tfar;

	ray_embree.flags = 0;
	ray_embree.time = 0;
	ray_embree.id = 0;
	ray_embree.mask = 0;
}

Ray::Ray(const Ray& ray_){
	ray_embree = ray_.ray_embree;
}

const glm::vec3& Ray::getOrg() const{
	return glm::vec3{ray_embree.org_x, ray_embree.org_y, ray_embree.org_z};
}

const glm::vec3& Ray::getDir() const{
	return glm::vec3{ray_embree.dir_x, ray_embree.dir_y, ray_embree.dir_z};
}

const glm::vec3& Ray::getRcpDir() const{
	return rcpDir;
}

const RTCRay& Ray::getRTCRay() const{
	return ray_embree;
}

glm::vec3 Ray::getHitPoint() const{
	return getOrg() + ray_embree.tfar * getDir();
}

void Ray::setRTCRay(const RTCRay& ray){
	this->ray_embree = ray;
}

RTCRay& Ray::getRTCRay(){
	return ray_embree;
}

void Ray::setOrg(const glm::vec3& newOrg){
	ray_embree.org_x = newOrg.x;
	ray_embree.org_y = newOrg.y;
	ray_embree.org_z = newOrg.z;
}

void Ray::setDir(const glm::vec3& newDir){
	ray_embree.dir_x = newDir.x;
	ray_embree.dir_y = newDir.y;
	ray_embree.dir_z = newDir.z;

	rcpDir = 1.0f / newDir;
}