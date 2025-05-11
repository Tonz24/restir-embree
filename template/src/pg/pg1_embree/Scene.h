#pragma once
#include "Sky.h"
#include "surface.h"
#include "TriangleCDF.h"

class Scene {
public:

	Scene() = default;

	Scene(const char* fileName, const RTCDevice& device);

	const RTCScene& getRTCScene() const;
	const Sky& getSkybox() const;
	const TriangleCDF& getEmissiveCDF() const;
	const std::vector<Surface*>& getSurfaces() const;
	const std::vector<Material*>& getMaterials() const;
	const std::vector<Triangle*>& getTriangles() const;

	void loadSkybox(const std::string& fileName);

	/*~Scene(){
		rtcReleaseScene(scene);
	}*/

private:
	RTCScene scene{};

	std::vector<Surface*> surfaces{};
	std::vector<Triangle*> triangles{};
	std::vector<Material*> materials{};
	std::unique_ptr<Sky> skybox{};
	TriangleCDF cdf;
};