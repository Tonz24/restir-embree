#pragma once
#include "Utils.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"


class CosineWeightedDistribution {
public:

	static glm::vec3 sample(const glm::vec3& normal) {
		float r1 = Utils::getRandomValue(0, 1);
		float r2 = Utils::getRandomValue(0, 1);

		float x = glm::cos(glm::pi<float>() * 2.0f * r1) * glm::sqrt(1.0f - r2);
		float y = glm::sin(glm::pi<float>() * 2.0f * r1) * glm::sqrt(1.0f - r2);
		float z = glm::sqrt(r2);

		//local reference frame
		glm::vec3 sample{ glm::normalize(glm::vec3{x,y,z }) };

		glm::vec3 o2 = glm::normalize(Utils::orthogonal(normal));
		glm::vec3 o1 = glm::normalize(glm::cross(normal, o2));
		o2 = glm::normalize(glm::cross(o1, normal));

		glm::mat3 toWorld{ o1,o2,normal };

		//transform to world space
		sample = toWorld * sample;

		return sample;
	}

	static float getPdf(const glm::vec3& normal, const glm::vec3& omega_i) {
		return glm::max(glm::dot(normal, omega_i), 0.0f) * glm::one_over_pi<float>();
	}

};


class CosineLobeDistribution {
public:

	static glm::vec3 sample(const glm::vec3& omega_r, float gamma) {
		float r1 = Utils::getRandomValue(0, 1);
		float r2 = Utils::getRandomValue(0, 1);

		float x = glm::cos(2.0f * glm::pi<float>() * r1) * glm::sqrt(1.0f - glm::pow(r2, 2.0f / (gamma + 1.0f)));
		float y = glm::sin(2.0f * glm::pi<float>() * r1) * glm::sqrt(1.0f - glm::pow(r2, 2.0f / (gamma + 1.0f)));
		float z = glm::pow(r2, 1.0f / (gamma + 1.0f));

		glm::vec3 sample{ glm::normalize(glm::vec3{x,y,z }) };

		glm::vec3 o2 = glm::normalize(Utils::orthogonal(omega_r));
		glm::vec3 o1 = glm::normalize(glm::cross(omega_r, o2));
		o2 = glm::normalize(glm::cross(o1, omega_r));

		const glm::mat3 toWorld{ o1,o2,omega_r };
		sample = toWorld * sample;

		float pdf = (gamma + 1.0f) * glm::one_over_two_pi<float>() * glm::pow(glm::max(0.0f, glm::dot(sample, omega_r)), gamma);

		return sample;
	}

	static float getPdf(const glm::vec3& omega_i, const glm::vec3& omega_r, float gamma = 0) {
		return (gamma + 1.0f) * glm::one_over_two_pi<float>() * glm::pow(glm::max(0.0f, glm::dot(omega_i, omega_r)), gamma);
	}

};