#ifndef CAMERA_H_
#define CAMERA_H_

#include "Ray.h"
#include "Texture.h"
#include "glm/glm.hpp"

#include "IDrawGui.h"
#include "PixelSampler.h"

/*! \class Camera
\brief A simple pin-hole camera.

\author Tomáš Fabián
\version 1.0
\date 2018
*/
class Camera : public IDrawGui {
public:
	Camera()  = default;

	Camera( const int width, const int height, const float fov_y,
		const glm::vec3 view_from, const glm::vec3 view_at, const Texture& tex);

	/* generate primary ray, top-left pixel image coordinates (xi, yi) are in the range <0, 1) x <0, 1) */
	Ray GenerateRay(const glm::vec2& org) const;

	void recalculate_m_c_w();
	void setPosition(const glm::vec3& newPos);

	void setViewAt(const glm::vec3& newViewAt) {
		viewAt = newViewAt;
		viewDir = glm::normalize(viewAt - view_from_);
		recalculate_m_c_w();
	}
	const glm::vec3& getPosition() const;
	const glm::vec3& getViewAt() const
	{
		return viewAt;
	}
	
	float getFocalLength() const;

	float getFOV() const;
	void setFOV(float newFOV);

	float apertureSize{ 0.0f };

	const glm::mat4& getViewMat() const {
		return viewMat;
	}

	const glm::mat4& getInvViewMat() const {
		return invViewMat;
	}

	
	void drawGui() override;

private:
	int width_{ 640 }; // image width (px)
	int height_{ 480 };  // image height (px)
	float fov_y_{ 0.785f }; // vertical field of view (rad)
	
	glm::vec3 view_from_; // ray origin or eye or O
	glm::vec3 viewAt{};
	glm::vec3 viewDir{};
	static constexpr glm::vec3 up_{0.0f, 0.0f, 1.0f}; // up vector

	glm::mat4 viewMat{};
	glm::mat4 invViewMat{};
	glm::mat3 invVievMatDir{};

	float f_y_{ 1.0f }; // focal lenght (px)

	//glm::mat3 M_c_w_{}; // transformation matrix from CS -> WS
	//glm::mat3 M_w_c_{}; // transformation matrix from WS -> CS

	const RTCScene* scene;

	void extractShape(const Texture& tex);

	std::vector<glm::vec2> shape{};

	std::unique_ptr<PixelSampler> pixelSampler{std::make_unique<CenterSampler>()};

	void calcMatrices();;
};



#endif
