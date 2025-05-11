#pragma once
#include "Texture.h"

class Sky{
public:
	virtual ~Sky() = default;

	Sky() = default;

	virtual glm::vec3 getTexel(const glm::vec3& dir) const = 0;
};