#pragma once
#include "glm/glm.hpp"

template<typename T>
class Texture {
public:

	explicit Texture(const std::string& fileName);


	const T& getTexel(const glm::vec<2, uint32_t>& coordsAbs){
		return getTexelInternal(coordsAbs);
	}
	const T& getTexel(const glm::vec<2, float>& coordsUV);

	uint32_t getWidth() const {
		return width;
	}
	uint32_t getHeight() const {
		return height;
	}


private:
	uint32_t width{0};
	uint32_t height{0};

	std::vector<T> data{};

	const T& getTexelInternal(const glm::vec<2, uint32_t>& coordsAbs){
		assert(coordsAbs.x >= 0 && coordsAbs.x < width);
		assert(coordsAbs.y >= 0 && coordsAbs.y < height);

		return data.at(coordsAbs.y * width + coordsAbs.x);
	}
};