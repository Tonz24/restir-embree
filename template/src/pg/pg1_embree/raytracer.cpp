#include "stdafx.h"
#include "raytracer.h"

#include <iostream>
#include <random>

#include "ModelLoader.h"
#include "NaivePathIntegrator.h"
#include "Ray.h"
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"

bool Raytracer::gammaCorrect{true};

Raytracer::Raytracer( int width, int height, float fov_y, const glm::vec3& view_from, const glm::vec3& view_at,
	const char * config ) : SimpleGuiDX11( width, height ) {

	camera_ = Camera{ width, height, fov_y, view_from, view_at};
	newCamPos = camera_.getPosition();
}

Raytracer::~Raytracer(){
	ReleaseDevice();
}



int Raytracer::ReleaseDevice(){
	rtcReleaseDevice( device_ );

	return S_OK;
}

void Raytracer::LoadScene( const std::string file_name ){
	scene = Scene{ file_name.c_str(), device_ };
	const auto path = std::string("../../../data/env/").append("pool.exr");
	scene.loadSkybox(path);
}

glm::vec3 Raytracer::get_pixel(const int x, const int y, const float t){

	Ray ray{ camera_.GenerateRay(glm::vec2{x,y})};

	const glm::vec3 radiance = currentIntegrator->integrate(ray, scene, {x,y});
	return radiance;
}

void Raytracer::drawGui() {
	 SimpleGuiDX11::drawGui();

	ImGui::Separator();

	camera_.drawGui();
	renderParams.drawGui();

	if (ImGui::CollapsingHeader("Statistics",ImGuiTreeNodeFlags_CollapsingHeader)) {
		ImGui::Text("Surfaces = %d", scene.getSurfaces().size());
		ImGui::Text("Materials = %d", scene.getMaterials().size());

		ImGui::Separator();

		ImGui::Text("Accumulator mean = %f", accumulatorMean);
		ImGui::Text("Accumulator variance = %f", accumulatorVariance);

		ImGui::Separator();

		ImGui::Text("G buffer fill: %lld ms", gBUfferFillDuration);
		ImGui::Text("Initial candidates generation: %lld ms", initialCandidatesGenDuration);
		ImGui::Text("Visibility pass: %lld ms", visibilityPassDuration);
		ImGui::Text("Temporal reuse pass: %lld ms", temporalReusePassDuration);
		ImGui::Text("Spatial reuse pass: %lld ms", spatialReusePassDuration);
		ImGui::Text("Shading pass: %lld ms", shadingPassDuration);
		ImGui::Text("Buffer copy duration: %lld ms", bufferCopyDuration);
		ImGui::Text("Total frame time: %lld ms", totalFrameDuration);
	}

	ImGui::End();
}