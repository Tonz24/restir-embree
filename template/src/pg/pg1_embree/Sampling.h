#pragma once
#include "triangle.h"
#include "glm/vec3.hpp"


struct TrianglePointSample{
	glm::vec3 samplePoint{};
	glm::vec3 normal{};
	const Triangle* triangle;
	float pdf{};
};

class Sampling {
public:
	Sampling() = delete;

	//returns <sample, pdf value>
	static std::pair<glm::vec3,float>sampleHemisphereCosineWeighted(const glm::vec3& normal);
	static float getPdfHemisphereCosineWeighted(const glm::vec3& normal, const glm::vec3 omega_i);

	//returns <sample, pdf value>
	static std::pair<glm::vec3,float> sampleCosineLobe(const glm::vec3& omega_r, float gamma);

	//returns <sample, normal, pdf value>
	static TrianglePointSample sampleTriangle(const Triangle& triangle);

	static glm::vec2 sampleDiskUniform(float radius);

	//returns <sample, pdf value>
	static std::pair<glm::vec3, float> sampleGGX(const glm::vec3& omega_h, float alpha);

	static std::pair<glm::vec3, float> sampleGGXVNDF(const glm::vec3& Ve, float alpha, float G1, float D);
};