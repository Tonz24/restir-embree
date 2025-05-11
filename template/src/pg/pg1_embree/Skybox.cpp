#include "stdafx.h"
#include "Skybox.h"

Skybox::Skybox(const char* path){

	std::string p{ path };

	for (int i = 0; i < 6; ++i){
		textures.push_back(std::make_unique<Texture>((p + names[i]).c_str()));
	}
}

//TODO: finish later
glm::vec3 Skybox::getTexel(const glm::vec3& dir) const {

	float maxValue{ abs(dir.x) };
	int index{ 0 }, sign{ 1 };

	for (int i = 1; i < 3; ++i) {

		if (abs(dir[i]) > maxValue) {

			maxValue = abs(dir[i]);
			index = i;
			sign = dir[i] > 0 ? 1 : 0;
		}
	}
	const auto& tex = textures[index * 2 - sign * 1];

	float u{ 0 }, v{ 0 };

	return tex->get_texel({ u,v });
}