#pragma once
#include "Sky.h"
#include "TextureDepr.h"
#include "glm/glm.hpp"

class SphericalMap : public Sky{
public:
	explicit SphericalMap(const char* path){
		texture = std::make_unique<TextureDepr>(path);
	}

	glm::vec3 getTexel(const glm::vec3& dir) const override;

private:
	std::unique_ptr<TextureDepr> texture{};
};