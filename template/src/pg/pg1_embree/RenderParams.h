#pragma once
#include "glm/glm.hpp"
#include "IDrawGui.h"

struct RenderParams : IDrawGui{
	void drawGui() override;

	uint32_t maxBounceCount{ 5 };
	glm::vec3 bgColor{0.5};
	bool useSkybox{ true };

	bool tonemap{ true };
	bool denoise{ false };

	float tnearOffset{ 0.01f };
	float tfarOffset{ 0.001f };
	float normalOffset{ 0.001f };
};