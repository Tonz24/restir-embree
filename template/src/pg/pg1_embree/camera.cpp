#include "stdafx.h"
#include "camera.h"

#include <corecrt_math_defines.h>

#include "raytracer.h"
#include "Sampling.h"
#include "Utils.h"
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"

Camera::Camera( const int width, const int height, const float fov_y, const glm::vec3 view_from, const glm::vec3 view_at, const Texture& tex) :
	width_(width), height_(height), fov_y_(fov_y), view_from_(view_from), viewAt(view_at), viewDir(glm::normalize(view_at - view_from)) {

	setFOV(fov_y);
	recalculate_m_c_w();

	extractShape(tex);
}

Ray Camera::GenerateRay(const glm::vec2& org) const {

	const auto sample = pixelSampler->takeSample();
	glm::vec2 orgShifted{org + sample};

	orgShifted = org;

	//generate world space ray direction
	glm::vec3 d_c{orgShifted.x - static_cast<float>(width_) / 2.0f, static_cast<float>(height_) / 2.0f - orgShifted.y, -f_y_};
	glm::vec3 d_w = invVievMatDir * d_c;
	d_w = glm::normalize(d_w);

	glm::vec3 offset{Sampling::sampleDiskUniform(apertureSize),0};
	offset = glm::mat3(invViewMat) * offset;

	//get point at focal length
	//glm::vec3 P = view_from_ + d_w * f_y_;

	//set ray's direction to pass through P from offset origin
	//glm::vec3 offsetOrigin = view_from_ + offset;
	//glm::vec3 dir = glm::normalize(P - offsetOrigin);

	//return Ray{ offsetOrigin , dir };
	return Ray{ view_from_ , d_w};
}

void Camera::recalculate_m_c_w(){

	glm::vec3 z_c = glm::normalize(view_from_ - viewAt);

	glm::vec3 x_c = glm::cross(up_, z_c);
	x_c = glm::normalize(x_c);

	glm::vec3 y_c = glm::cross(z_c, x_c);
	y_c = glm::normalize(y_c);

	viewMat = glm::lookAt(view_from_, viewAt, y_c);
	invViewMat = glm::inverse(viewMat);
	invViewMat = glm::inverse(viewMat);
	invVievMatDir = glm::mat3(invViewMat);
}

void Camera::setPosition(const glm::vec3& newPos){
	view_from_ = newPos;
	viewDir = glm::normalize(viewAt - view_from_);
	recalculate_m_c_w();
}

const glm::vec3& Camera::getPosition() const{
	return view_from_;
}


float Camera::getFocalLength() const
{
	return f_y_;
}

float Camera::getFOV() const{

	return glm::degrees(fov_y_);
}

void Camera::setFOV(float newFOV){
	fov_y_ = glm::radians(newFOV);
	f_y_ = static_cast<float>(height_) / (2.0f * tanf(fov_y_ / 2.0f));
}

void Camera::drawGui() {
	if (ImGui::CollapsingHeader("Camera options")){

		float newFov{ glm::degrees(fov_y_) };
		if (ImGui::DragFloat("Vertical FOV", &newFov, 1, 5, 179.99)) {
			setFOV(newFov);
		}

		ImGui::DragFloat("Aperture size", &apertureSize, 0.03, 0, 100);

		std::string pos{ "Camera position " + glm::to_string(view_from_) };
		ImGui::Text(pos.c_str());

		std::string viewDir{ "View direction " + glm::to_string(this->viewDir) };
		ImGui::Text(viewDir.c_str());

		if (ImGui::CollapsingHeader("Anti aliasing options")) {
			static const char* itemsSampler[]{ "Center Sampling", "Random Sampling", "Stratified Jittered Sampling" };
			static int samplerIndex = 0;

			std::unique_ptr<PixelSampler> newSampler{};
			if (ImGui::Combo("Pixel sampling strategy", &samplerIndex, itemsSampler, IM_ARRAYSIZE(itemsSampler))) {
				switch (samplerIndex) {
				case 1:
					newSampler = std::make_unique<RandomSampler>();
					std::swap(pixelSampler, newSampler);
					break;
				case 2:
					newSampler = std::make_unique<StratifiedJitteredSampler>();
					std::swap(pixelSampler, newSampler);
					break;

				default:
					newSampler = std::make_unique<CenterSampler>();
					std::swap(pixelSampler, newSampler);
					break;
				}
			}
			pixelSampler->drawGui();
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Separator();
	}
}

void Camera::extractShape(const Texture& tex){
	for (int x = 0; x < tex.width(); ++x){
		float u = static_cast<float>(x) / static_cast<float>(tex.width()) * 2.0f - 1.0f;

		for (int y = 0; y < tex.height(); ++y) {
			float v = 1.0f - static_cast<float>(y) / static_cast<float>(tex.height()) * 2.0f - 1.0f;

			glm::vec3 color = tex.get_texel(x, y);
			if (0.299 * color.x + 0.587 * color.y + 0.114 * color.z > 0.5)
				shape.push_back({ u,v });
		}
	}
}
