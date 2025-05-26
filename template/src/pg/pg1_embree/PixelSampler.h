#pragma once
#include "IDrawGui.h"
#include "Utils.h"
#include "glm/glm.hpp"

class PixelSampler : public IDrawGui{
public:
	void drawGui() override{}
	virtual glm::vec2 takeSample() const = 0;
};

class CenterSampler : public PixelSampler {
public:
	glm::vec2 takeSample() const override {
		return glm::vec2{ 0.0 };
	}
};


class RandomSampler: public PixelSampler {
public:
	glm::vec2 takeSample() const override {
		float x = Utils::getRandomValue(0, 1);
		float y = Utils::getRandomValue(0, 1);
		return glm::vec2{x, y};
	}
};

class StratifiedJitteredSampler : public PixelSampler {
public:

	StratifiedJitteredSampler(const glm::vec<2, int>& gridDims = glm::vec<2,int>{5,5}){
		setGridDims(gridDims);
	}

	glm::vec2 takeSample() const override {
		//first scale the offset within a block
		float x = Utils::getRandomValue(0, 1) * blockSizes.x;
		float y = Utils::getRandomValue(0, 1) * blockSizes.y;
		glm::vec2 blockOffset{ x, y };

		//then pick a block
		float blockX = glm::floor(Utils::getRandomValue(0, 1) * static_cast<float>(gridDims.x)) * blockSizes.x;
		float blockY = glm::floor(Utils::getRandomValue(0, 1) * static_cast<float>(gridDims.y)) * blockSizes.y;

		//to the selected block corner, add the block offset
		return glm::vec2{blockX + blockOffset.x, blockY + blockOffset.y};
	}

	void setGridDims(const glm::vec2& newDims){
		gridDims = newDims;
		calculateBlockSizes();
	}

	void drawGui() override {
		if (ImGui::DragInt2("Grid dimensions", &gridDims[0], 1, 1, 100))
			calculateBlockSizes();
	}

private:
	glm::vec<2,int> gridDims{ 5,5 };
	glm::vec2 blockSizes{ 5,5 };

	void calculateBlockSizes() {
		blockSizes = 1.0f / static_cast<glm::vec2>(gridDims);
	}
};