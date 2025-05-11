#include "stdafx.h"
#include "MaterialPhong.h"

#include <iostream>

#include "raytracer.h"
#include "Sampling.h"
#include "Utils.h"
#include "glm/gtc/constants.hpp"

#include <boost/math/special_functions/beta.hpp>


//phong: multiply pdf with (diffuse / diffuse + specular) if diffuse, or 1 - that if specular
//cosine lobe sampling: be careful about coordinate space, make the same matrix as TBN, but use omega_r as the normal, return zero if sampled direction points INTO the surface, normalization factor
//original Phong: gamma + 1, modified Phong: gamma + 2
//plast: 0.05 specular, kov: jenom specular, žádný diffuse
PTInfoGI MaterialPhong::evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const {

	//gather needed values
	glm::vec3 diffuse = getDiffuseColor(hitInfo.uv);
	glm::vec3 specular = getSpecularColor(hitInfo.uv);
	float shininess = getShininess(hitInfo.uv);

	//hope that diffuse + specular = 1
	float maxDiffuse = Utils::maxComponent(diffuse);
	float maxSpecular = Utils::maxComponent(specular);

	float r0 = Utils::getRandomValue(0.0f, maxDiffuse + maxSpecular);
	float pdfFactor{ maxDiffuse / (maxDiffuse + maxSpecular)};

	glm::vec3 omega_i{};
	const glm::vec3 omega_r = glm::normalize(glm::reflect(ray.getDir(), hitInfo.normal));

	glm::vec3 f_r{};
	VertexType vertexType{};

	//standard cosine weighted diffuse sample
	if (r0 < maxDiffuse) {
		omega_i = CosineWeightedDistribution::sample(hitInfo.normal);
		f_r = diffuse * glm::one_over_pi<float>();
		vertexType = DIFFUSE_VERTEX;
	}

	//sample specular lobe
	else {

		omega_i = CosineLobeDistribution::sample(omega_r, shininess);

		//Mallett’s and Yuksel’s method for energy normalization
		float nDotV = glm::dot(-ray.getDir(), hitInfo.normal);
		float i_m = 1.0f / calc_I_M(nDotV, shininess);

		f_r = specular * i_m * glm::pow(glm::max(glm::dot(omega_i, omega_r), 0.0f), shininess);
		vertexType = SPECULAR_VERTEX;
	}

	float pdfDiffuse = CosineWeightedDistribution::getPdf(hitInfo.normal, omega_i) * pdfFactor;
	float pdfSpecular = CosineLobeDistribution::getPdf(omega_i, omega_r, shininess) * (1.0f - pdfFactor);
	float pdf = pdfDiffuse + pdfSpecular;

	//when omega_i points INTO the surface, exit; BRDF value f_r = 0, but integrate the sample nonetheless
	if (glm::dot(hitInfo.normal, omega_i) < 0)
		return { omega_i, glm::vec3{0}, pdf};

	return { omega_i, f_r, pdf, vertexType};
}

BRDFEval MaterialPhong::evaluateBRDF(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const {
	//gather needed values
	//hope that diffuse + specular = 1
	glm::vec3 diffuse = getDiffuseColor(hitInfo.uv);
	glm::vec3 specular = getSpecularColor(hitInfo.uv);
	float shininess = getShininess(hitInfo.uv);

	///////////////////////////////////////////////////////////////////
	//diffuse part of BRDF
	glm::vec3 f_r = diffuse * glm::one_over_pi<float>();
	///////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////
	//specular part of BRDF
	//Mallett’s and Yuksel’s method for energy normalization
	float nDotV = glm::dot(-ray.getDir(), hitInfo.normal);
	float i_m = 1.0f / calc_I_M(nDotV, shininess);

	const glm::vec3 omega_r = glm::normalize(glm::reflect(ray.getDir(), hitInfo.normal));
	f_r += specular * i_m * glm::pow(glm::max(glm::dot(omega_i, omega_r), 0.0f), shininess);
	///////////////////////////////////////////////////////////////////

	return {f_r};
}

float MaterialPhong::getPdfForSample(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const
{
	//gather needed values
	glm::vec3 diffuse = getDiffuseColor(hitInfo.uv);
	glm::vec3 specular = getSpecularColor(hitInfo.uv);
	float shininess = getShininess(hitInfo.uv);

	//hope that diffuse + specular = 1
	float maxDiffuse = Utils::maxComponent(diffuse);
	float maxSpecular = Utils::maxComponent(specular);

	//float r0 = Utils::getRandomValue(0.0f, maxDiffuse + maxSpecular);
	float pdfFactor{ maxDiffuse / (maxDiffuse + maxSpecular) };

	//standard cosine weighted diffuse sample
	//if (r0 < maxDiffuse)
	float pdf = CosineWeightedDistribution::getPdf(hitInfo.normal,omega_i) * pdfFactor;

	//sample specular lobe
	//else {
	const glm::vec3 omega_r = glm::normalize(glm::reflect(ray.getDir(), hitInfo.normal));
	pdf += CosineLobeDistribution::getPdf(omega_i,omega_r, shininess) * (1.0f - pdfFactor);
	//}

	return pdf;
}


glm::vec3 MaterialPhong::evalBRDF(const GBufferElement& gBufferElement, const glm::vec3& cameraPos,
	const glm::vec3& omega_i) {

	const glm::vec3 V = glm::normalize(cameraPos - gBufferElement.worldSpacePos);

	//gather needed values
	//hope that diffuse + specular = 1
	glm::vec3 diffuse = gBufferElement.diffuseColor;
	glm::vec3 specular = gBufferElement.specularColor;

	///////////////////////////////////////////////////////////////////
	//diffuse part of BRDF
	glm::vec3 f_r = diffuse * glm::one_over_pi<float>();
	///////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////
	//specular part of BRDF
	//Mallett’s and Yuksel’s method for energy normalization
	float nDotV = glm::dot(V, gBufferElement.worldSpaceNormal);
	float i_m = 1.0f / calc_I_M(nDotV, gBufferElement.shininess);

	const glm::vec3 omega_r = glm::normalize(glm::reflect(-V, gBufferElement.worldSpaceNormal));
	f_r += specular * i_m * glm::pow(glm::max(glm::dot(omega_i, omega_r), 0.0f), gBufferElement.shininess);
	///////////////////////////////////////////////////////////////////

	return f_r;
}

float MaterialPhong::evalPdf(const GBufferElement& gBufferElement, const glm::vec3& cameraPos, const glm::vec3& omega_i) {
	//gather needed values
	glm::vec3 diffuse = gBufferElement.diffuseColor;
	glm::vec3 specular = gBufferElement.specularColor;

	//hope that diffuse + specular = 1
	float maxDiffuse = Utils::maxComponent(diffuse);
	float maxSpecular = Utils::maxComponent(specular);

	float pdfFactor{ maxDiffuse / (maxDiffuse + maxSpecular) };

	//standard cosine weighted diffuse sample
	//if (r0 < maxDiffuse)
		float pdf = CosineWeightedDistribution::getPdf(gBufferElement.worldSpaceNormal, omega_i) * pdfFactor;

	//sample specular lobe
	//else {
		const glm::vec3 omega_o = glm::normalize(gBufferElement.worldSpacePos - cameraPos);
		const glm::vec3 omega_r = glm::normalize(glm::reflect(omega_o, gBufferElement.worldSpaceNormal));
		pdf += CosineLobeDistribution::getPdf( omega_i, omega_r, gBufferElement.shininess) * (1.0f - pdfFactor);
	//}
	return pdf;
}

PTInfoGI MaterialPhong::sampleBRDF(const GBufferElement& gBufferElement, const glm::vec3& cameraPos){
	//gather needed values
	glm::vec3 diffuse = gBufferElement.diffuseColor;
	glm::vec3 specular = gBufferElement.specularColor;

	glm::vec3 omega_o = glm::normalize(gBufferElement.worldSpacePos - cameraPos);

	//hope that diffuse + specular = 1
	float maxDiffuse = Utils::maxComponent(diffuse);
	float maxSpecular = Utils::maxComponent(specular);

	float r0 = Utils::getRandomValue(0.0f, maxDiffuse + maxSpecular);
	float pdfFactor{ maxDiffuse / (maxDiffuse + maxSpecular) };

	glm::vec3 omega_i{};
	const glm::vec3 omega_r = glm::normalize(glm::reflect(omega_o, gBufferElement.worldSpaceNormal));

	glm::vec3 f_r{};
	VertexType vertexType{};

	//standard cosine weighted diffuse sample
	if (r0 < maxDiffuse) {
		omega_i = CosineWeightedDistribution::sample(gBufferElement.worldSpaceNormal);
		f_r = diffuse * glm::one_over_pi<float>();
		vertexType = DIFFUSE_VERTEX;
	}

	//sample specular lobe
	else {
		omega_i = CosineLobeDistribution::sample(omega_r, gBufferElement.shininess);

		//Mallett’s and Yuksel’s method for energy normalization
		float nDotV = glm::dot(-omega_o, gBufferElement.worldSpaceNormal);
		float i_m = 1.0f / calc_I_M(nDotV, gBufferElement.shininess);

		f_r = specular * i_m * glm::pow(glm::max(glm::dot(omega_i, omega_r), 0.0f), gBufferElement.shininess);
		vertexType = SPECULAR_VERTEX;
	}

	float pdfDiffuse = CosineWeightedDistribution::getPdf(gBufferElement.worldSpaceNormal, omega_i) * pdfFactor;
	float pdfSpecular = CosineLobeDistribution::getPdf(omega_i, omega_r, gBufferElement.shininess) * (1.0f - pdfFactor);
	float pdf = pdfDiffuse + pdfSpecular;

	//when omega_i points INTO the surface, exit; BRDF value f_r = 0, but integrate the sample nonetheless
	if (glm::dot(gBufferElement.worldSpaceNormal, omega_i) < 0)
		return { omega_i, glm::vec3{0}, pdf };

	return { omega_i, f_r, pdf, vertexType };
}

float MaterialPhong::gamma_quot(float a, float b) {
	return std::exp(std::lgamma(a) - std::lgamma(b));
}

float MaterialPhong::calc_I_M(float nDotV, float n){

	float const& costerm = nDotV;
	float sinterm_sq = 1.0f - costerm * costerm;
	float halfn = 0.5f * n;

	float negterm = costerm;

	//clamp so that floating point magic doesn't put the value outside [0,1] range
	sinterm_sq = glm::clamp(sinterm_sq, 0.0f, 1.0f);

	if (n >= 1e-18f)
		negterm *= halfn * ibeta(sinterm_sq,halfn, 0.5f);

	return	(glm::two_pi<float>() * costerm + glm::root_pi<float>() * gamma_quot(halfn + 0.5f, halfn + 1.0f) * (std::pow(sinterm_sq,halfn) - negterm)) /
			(n + 2.0f);
}

float MaterialPhong::ibeta(float x, float a, float b){
	return boost::math::beta(a, b, x);
}
