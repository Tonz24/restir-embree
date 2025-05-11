#include "stdafx.h"
#include "material.h"


const char Material::kDiffuseMapSlot = 0;
const char Material::kSpecularMapSlot = 1;
const char Material::kNormalMapSlot = 2;
const char Material::kShininessMapSlot = 3;

Material::Material()
{
	// defaultní materiál
	ambient = glm::vec3( 0.1f, 0.1f, 0.1f );
	diffuse = glm::vec3( 0.4f, 0.4f, 0.4f );
	specular = glm::vec3( 0.8f, 0.8f, 0.8f );	

	emission = glm::vec3( 0.0f, 0.0f, 0.0f );	

	reflectivity = static_cast<float>( 0.99 );
	shininess_ = 1;

	ior = -1;

	memset( textures_, 0, sizeof( textures_));

	name_ = "default";
}

Material::Material( std::string & name, const glm::vec3 & ambient, const glm::vec3 & diffuse,
	const glm::vec3 & specular, const glm::vec3 & emission, const float reflectivity, 
	const float shininess, const float ior, Texture ** textures, const int no_textures )
{
	name_ = name;

	this->ambient = ambient;
	this->diffuse = diffuse;
	this->specular = specular;

	this->emission = emission;

	this->reflectivity = reflectivity;
	this->shininess_ = shininess;	

	this->ior = ior;

	if ( textures )
	{
		memcpy( textures_, textures, sizeof( textures ) * no_textures );
	}
}

Material::~Material()
{
	for ( int i = 0; i < NO_TEXTURES; ++i )
	{
		if ( textures_[i] )
		{
			delete[] textures_[i];
			textures_[i] = nullptr;
		};
	}
}

void Material::set_name( const char * name )
{
	name_ = std::string( name );
}

std::string Material::get_name() const
{
	return name_;
}

void Material::set_texture( const int slot, Texture * texture )
{
	textures_[slot] = texture;
}

Texture * Material::get_texture( const int slot ) const
{
	return textures_[slot];
}

PTInfoGI Material::evaluateLightingGI(const HitInfo& hitInfo, const Ray& ray) const{
	return {};
}

BRDFEval Material::evaluateBRDF(const HitInfo& hitInfo, const Ray& ray, const glm::vec3& omega_i) const {
	return {};
}

bool Material::isEmitter() const {
	return emission.x > 0.0f || emission.y > 0.0f || emission.z > 0.0f;
}


bool Material::recurse() const{
	return false;
}

MaterialType Material::getType() const{
	return UNSUPPORTED;
}

glm::vec3 Material::getDiffuseColor(const glm::vec2& uv) const
{
	glm::vec3 diff = diffuse;
	const auto texture = get_texture(kDiffuseMapSlot);
	if (texture)
		diff = texture->get_texel(uv);
	return diff;
}

glm::vec3 Material::getSpecularColor(const glm::vec2& uv) const
{
	glm::vec3 spec = specular;
	const auto texture = get_texture(kSpecularMapSlot);
	if (texture)
		spec = texture->get_texel(uv);
	return spec;
}

float Material::getShininess(const glm::vec2& uv) const{

	const auto texture = get_texture(kShininessMapSlot);
	if (texture) {
		auto texel = texture->get_texel(uv);

		//convert roughness to shininess_
		float s = 2.0f / (texel.x * texel.x) - 2.0f; 
		return s;
	}
	return shininess_;
}
