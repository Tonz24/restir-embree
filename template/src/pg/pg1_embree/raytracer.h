#pragma once
#include "Integrator.h"
#include "PixelSampler.h"
#include "RenderParams.h"
#include "Reservoir.h"
#include "Scene.h"
#include "simpleguidx11.h"

class Raytracer : public SimpleGuiDX11
{
public:
	Raytracer(int width, int height, float fov_y, const glm::vec3& view_from, const glm::vec3& view_at,
	          const char * config = "threads=0,verbose=3" );
	~Raytracer();

	int ReleaseDevice();

	void LoadScene( const std::string file_name );

	glm::vec3 get_pixel(const int x, const int y, const float t = 0.0f) override;

	void drawGui() override;

	static bool gammaCorrect;


private:
	std::unique_ptr<PixelSampler> pixelSampler{};
};