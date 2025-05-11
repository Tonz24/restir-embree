#include "stdafx.h"
#include "Sampling.h"

#include "DirectAreaIntegrator.h"
#include "DirectAreaIntegrator.h"
#include "surface.h"
#include "Utils.h"
#include "glm/glm.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtc/constants.hpp"

std::pair<glm::vec3, float> Sampling::sampleHemisphereCosineWeighted(const glm::vec3& normal){
	float r1 = Utils::getRandomValue(0, 1);
	float r2 = Utils::getRandomValue(0, 1);

	float x = glm::cos(glm::pi<float>() * 2.0f * r1) * glm::sqrt(1.0f - r2);
	float y = glm::sin(glm::pi<float>() * 2.0f * r1) * glm::sqrt(1.0f - r2);
	float z = glm::sqrt(r2);

	//local reference frame
	glm::vec3 sample{glm::normalize(glm::vec3{x,y,z })};

	glm::vec3 o2 = glm::normalize(Utils::orthogonal(normal));
	glm::vec3 o1 = glm::normalize(glm::cross(normal, o2));
	o2 = glm::normalize(glm::cross(o1, normal));

	glm::mat3 toWorld{ o1,o2,normal };

	//transform to world space
	sample = toWorld * sample;

	float pdf = glm::max(glm::dot(normal, sample), 0.0f) * glm::one_over_pi<float>();

	return {sample,pdf};
}

float Sampling::getPdfHemisphereCosineWeighted(const glm::vec3& normal, const glm::vec3 omega_i) {
	return  glm::max(glm::dot(normal, omega_i), 0.0f) * glm::one_over_pi<float>();
}

std::pair<glm::vec3, float> Sampling::sampleCosineLobe(const glm::vec3& omega_r, float gamma){
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

	float pdf = (gamma + 1.0f) * glm::one_over_two_pi<float>() * glm::pow(glm::max(0.0f,glm::dot(sample,omega_r)) ,gamma);

	return { sample, pdf };
}

TrianglePointSample Sampling::sampleTriangle(const Triangle& triangle) {

	float r1 = Utils::getRandomValue(0, 1);
	float r2 = Utils::getRandomValue(0, 1);

	float x = 1.0f - glm::sqrt(r1);
	float y = glm::sqrt(r1) * (1.0f - r2);
	float z = glm::sqrt(r1) * r2;

	glm::vec3 sample = triangle[0].position * x + triangle[1].position * y + triangle[2].position * z;
	glm::vec3 normal =  glm::normalize(triangle[0].normal * x + triangle[1].normal * y + triangle[2].normal * z);
	float pdf = 1.0f / triangle.getSurfaceArea();
	return { sample, normal, &triangle,pdf};
}

glm::vec2 Sampling::sampleDiskUniform(float radius) {

	float theta = Utils::getRandomValue(0,2.0f) * glm::pi<float>();
	float r = glm::sqrt(Utils::getRandomValue(0,radius));

	float x = r * glm::cos(theta);
	float y = r * glm::sin(theta);

	return { x,y };
}


//todo: later
//omega_i = refl(omega_o, omega_m) -> omega_m vyleze z GGX (normala mikroplosky)
//pdf(omega_i) = p(omega_m) prenasobit srandou z prezentace (rozbite dolni indexy v prezentaci)
//hodne vzorku ma nulovou hodnotu, proto VNDF (vzorkuji se pouze viditelne normaly)
//obslehnout VNDF pseudokod
std::pair<glm::vec3, float> Sampling::sampleGGX(const glm::vec3& omega_h, float alpha) {
	float r1 = Utils::getRandomValue(0, 1);

	float theta_n = glm::atan(alpha * glm::sqrt(r1 / (1.0f - r1)));
	float pdf{};


	return {{}, {}};
}

std::pair<glm::vec3, float> Sampling::sampleGGXVNDF(const glm::vec3& Ve, float alpha, float G1, float D) {
	float u1 = Utils::getRandomValue(0.0f, 1.0f);
	float u2 = Utils::getRandomValue(0.0f, 1.0f);

	//just to not get confused
	const float alpha_x = alpha;
	const float alpha_y = alpha;

	const glm::vec3 Vh = glm::normalize(glm::vec3{ Ve.x * alpha_x, Ve.y * alpha_y, Ve.z });

	float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
	glm::vec3 T1 = lensq > 0 ? glm::vec3{ -Vh.y, Vh.x, 0 } *glm::inversesqrt(lensq) : glm::vec3{ 1,0,0 };
	glm::vec3 T2 = glm::cross(Vh, T1);

	float r = glm::sqrt(u1);
	float phi = 2.0 * glm::pi<float>() * u2;
	float t1 = r * glm::cos(phi);
	float t2 = r * glm::sin(phi);
	float s = 0.5 * (1.0 + Vh.z);
	t2 = (1.0 - s) * glm::sqrt(1.0 - t1 * t1) + s * t2;

	glm::vec3 Nh = t1 * T1 + t2 * T2 + static_cast<float>(glm::sqrt(glm::max(0.0, 1.0 - t1*t1 - t2*t2))) * Vh;

	glm::vec3 Ne = glm::normalize(glm::vec3{ alpha_x * Nh.x, alpha_y * Nh.y, glm::max(0.0f,Nh.z)});

	//G1(Ve) D(Ne)
	float pdf = G1 * glm::max(0.0f, glm::dot(Ve,Ne)) * D / Ve.z;

	return { Ne,pdf };
}