#include "stdafx.h"
#include "SphericalMap.h"

#include <corecrt_math_defines.h>

#include "raytracer.h"

#define INVPI (1.0 / M_PI)

glm::vec3 SphericalMap::getTexel(const glm::vec3& dir) const{
	const float x = 0.5f + 0.5f * atan2f(dir.y, dir.x) * INVPI;
	const float y = 1.0f - acos(dir.z) * INVPI;
	return texture->get_texel({ x,y });
}