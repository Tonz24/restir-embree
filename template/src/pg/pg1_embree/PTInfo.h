#pragma once
#include "enums.h"
#include "glm/glm.hpp"

struct PTInfoGI {
	glm::vec3 omega_i{};
	glm::vec3 f_r{};
	float pdf{};
	VertexType vertexType{ INVALID_VERTEX };
};

struct BRDFEval {
	glm::vec3 f_r{ 0 };
};