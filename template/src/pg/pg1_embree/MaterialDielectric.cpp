#include "stdafx.h"
#include "MaterialDielectric.h"
#include "Sampling.h"
#include "Utils.h"
#include "glm/gtc/constants.hpp"
#include <boost/math/special_functions/beta.hpp>


PTInfoGI MaterialDielectric::evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const{
	//gather needed values
	//hope that diffuse + specular = 1
	glm::vec3 diffuse = getDiffuseColor(hitInfo.uv);
	glm::vec3 specular = getSpecularColor(hitInfo.uv);
	float shininess = getShininess(hitInfo.uv);

	glm::vec3 specularReflectance = Utils::schlickApprox3(ray.getDir(), hitInfo.normal, specular);
	glm::vec3 diffuseReflectance = (1.0f - Utils::maxComponent(specularReflectance)) / (1.0f - Utils::maxComponent(specular)) * diffuse;

	float maxDiffuse = Utils::maxComponent(diffuseReflectance);
	float maxSpecular = Utils::maxComponent(specularReflectance);

	float r0 = Utils::getRandomValue(0.0f, maxDiffuse + maxSpecular);
	float pdfFactor{ maxDiffuse / (maxDiffuse + maxSpecular) };

	glm::vec3 omega_i{};
	const glm::vec3 omega_r = glm::normalize(glm::reflect(ray.getDir(), hitInfo.normal));

	glm::vec3 f_r{};
	VertexType vertexType{};


	//standard cosine weighted diffuse sample
	if (r0 < maxDiffuse) {
		omega_i = CosineWeightedDistribution::sample(hitInfo.normal);
		//pdf = diffuseSample.pdf * pdfFactor;
		f_r = diffuseReflectance * glm::one_over_pi<float>();
		vertexType = DIFFUSE_VERTEX;
	}

	//sample specular lobe
	else {

		omega_i = CosineLobeDistribution::sample(omega_r, shininess);
		//pdf = specularSample.pdf * (1.0f - pdfFactor);

		//Mallett’s and Yuksel’s method for energy normalization
		float nDotV = glm::dot(-ray.getDir(), hitInfo.normal);
		float i_m = 1.0f / calc_I_M(nDotV, shininess);

		f_r = specularReflectance * i_m * glm::pow(glm::max(glm::dot(omega_i, omega_r), 0.0f), shininess);
		vertexType = SPECULAR_VERTEX;
	}

	float pdfDiffuse = CosineWeightedDistribution::getPdf(hitInfo.normal, omega_i) * pdfFactor;
	float pdfSpecular = CosineLobeDistribution::getPdf(omega_i, omega_r, shininess) * (1.0f - pdfFactor);
	float pdf = pdfDiffuse + pdfSpecular;

	//when omega_i points INTO the surface, exit; BRDF value f_r = 0, but integrate the sample nonetheless
	if (glm::dot(hitInfo.normal, omega_i) < 0)
		return { omega_i, glm::vec3{0}, pdf };

	return { omega_i, f_r, pdf, vertexType };
}

BRDFEval MaterialDielectric::evaluateBRDF(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const{
	//gather needed values
	//hope that diffuse + specular = 1
	glm::vec3 diffuse = getDiffuseColor(hitInfo.uv);
	glm::vec3 specular = getSpecularColor(hitInfo.uv);
	float shininess = getShininess(hitInfo.uv);

	glm::vec3 specularReflectance = Utils::schlickApprox3(ray.getDir(), hitInfo.normal, specular);
	glm::vec3 diffuseReflectance = (1.0f - Utils::maxComponent(specularReflectance)) / (1.0f - Utils::maxComponent(specular)) * diffuse;

	///////////////////////////////////////////////////////////////////
	//diffuse part of BRDF
	glm::vec3 f_r = diffuseReflectance * glm::one_over_pi<float>();
	///////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////
	//specular part of BRDF
	float nDotV = glm::dot(-ray.getDir(), hitInfo.normal);
	float i_m = 1.0f / calc_I_M(nDotV, shininess);

	const glm::vec3 omega_r = glm::normalize(glm::reflect(ray.getDir(), hitInfo.normal));
	f_r += specularReflectance * i_m * glm::pow(glm::max(glm::dot(omega_i, omega_r), 0.0f), shininess);
	///////////////////////////////////////////////////////////////////

	return {f_r};
}

float MaterialDielectric::getPdfForSample(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const
{
	glm::vec3 diffuse = getDiffuseColor(hitInfo.uv);
	glm::vec3 specular = getSpecularColor(hitInfo.uv);
	float shininess = getShininess(hitInfo.uv);

	glm::vec3 specularReflectance = Utils::schlickApprox3(ray.getDir(), hitInfo.normal, specular);
	glm::vec3 diffuseReflectance = (1.0f - Utils::maxComponent(specularReflectance)) / (1.0f - Utils::maxComponent(specular)) * diffuse;

	float maxDiffuse = Utils::maxComponent(diffuseReflectance);
	float maxSpecular = Utils::maxComponent(specularReflectance);

	//float r0 = Utils::getRandomValue(0.0f, maxDiffuse + maxSpecular);
	float pdfFactor{ maxDiffuse / (maxDiffuse + maxSpecular) };

	//standard cosine weighted diffuse sample
	//if (r0 < maxDiffuse)
		float pdf = CosineWeightedDistribution::getPdf(hitInfo.normal, omega_i) * pdfFactor;
	//sample specular lobe
	//else {
		const glm::vec3 omega_r = glm::normalize(glm::reflect(ray.getDir(), hitInfo.normal));
		pdf += CosineLobeDistribution::getPdf(omega_i, omega_r, shininess) * (1.0f - pdfFactor);
	//}
	return pdf;
}
