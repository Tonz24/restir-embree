#pragma once
#include "Sky.h"

class Skybox: public Sky {
public:

	explicit Skybox(const char* path);
	glm::vec3 getTexel(const glm::vec3& dir) const override;

private:

	/*
	 * px, nx
	 * pz, nz
	 * py, ny
	 */
	std::vector<std::unique_ptr<TextureDepr>> textures;

	const char* names[6] = {"px.jpg","nx.jpg","pz.jpg" ,"nz.jpg" ,"py.jpg" ,"ny.jpg" };
};