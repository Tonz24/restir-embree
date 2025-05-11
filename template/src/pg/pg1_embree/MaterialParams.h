#pragma once
#include "glm/glm.hpp"

struct MaterialParams {
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	float ior;
	glm::vec3 transmissionAttenuation;

};

