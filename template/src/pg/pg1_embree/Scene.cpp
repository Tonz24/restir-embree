#include "stdafx.h"
#include "Scene.h"

#include "ModelLoader.h"
#include "SphericalMap.h"


Scene::Scene(const char* fileName, const RTCDevice& device)
{
	scene = rtcNewScene(device);

	auto tris = ModelLoader::loadScene(fileName, surfaces, materials, device, scene);
	triangles = tris.first;
	cdf = TriangleCDF(std::move(tris.second));
	rtcCommitScene(scene);
}

const RTCScene& Scene::getRTCScene() const
{
	return scene;
}

const Sky& Scene::getSkybox() const
{
	return *skybox;
}

const TriangleCDF& Scene::getEmissiveCDF() const
{
	return cdf;
}

const std::vector<Surface*>& Scene::getSurfaces() const{
	return surfaces;
}

const std::vector<Material*>& Scene::getMaterials() const {
	return materials;
}

const std::vector<Triangle*>& Scene::getTriangles() const
{
	return triangles;
}

void Scene::loadSkybox(const std::string& fileName)
{
	std::unique_ptr<Sky> newSky = std::make_unique<SphericalMap>(fileName.c_str());
	std::swap(skybox, newSky);
}
