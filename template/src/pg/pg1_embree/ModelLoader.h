#pragma once
#include <assimp/scene.h>

#include "surface.h"

class ModelLoader {
public:
	//no instantiation of this class
	ModelLoader() = delete;

	static void loadOBJ(const char* file_name, std::vector<Surface*>& surfaces, std::vector<Material*>& materials,
		const bool flip_yz = false, const glm::vec3 default_color = glm::vec3(0.5f, 0.5f, 0.5f));


	//returns: vector of all triangles (for bvh construction), vector of all emissive triangles (for emissive triangles cdf calculation)
	static std::pair<std::vector<Triangle*>, std::vector<Triangle*>> loadScene(const char* file_name, std::vector<Surface*>& surfaces, std::vector<Material*>& materials, const RTCDevice& device, const RTCScene& scene);

private:
	static void loadMaterials(const std::string& directory, const aiScene& scene, std::vector<Material*>& materials);
	static TextureDepr* TextureProxy(const std::string& full_name, std::map<std::string, TextureDepr*>& already_loaded_textures,
		const int flip = -1, const bool single_channel = false);
};