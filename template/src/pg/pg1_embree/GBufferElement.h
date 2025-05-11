#pragma once
#include "enums.h"
#include "glm/glm.hpp"


struct GBufferElement {
	glm::vec3 worldSpacePos{};
	glm::vec3 worldSpaceNormal{};

	glm::vec3 diffuseColor{};
	glm::vec3 specularColor{};
	glm::vec3 emission{};

	float shininess{};
	float depth{};

	MaterialType materialType{};

	//the hit surface is valid when there's no emission (light sources and environment maps are displayed directly)
	bool isValidForReSTIR() const{
		return emission.x == 0 && emission.y == 0 && emission.z == 0;
	}
};


class GBuffer {
public:

	GBuffer() = default;

	GBuffer(const glm::vec<2,int>& size) : size(size) {
		wSpacePositionBuf.resize(size.x * size.y);
		wSpaceNormalBuf.resize(size.x * size.y);

		diffuseColorBuf.resize(size.x * size.y);
		specularColorBuf.resize(size.x * size.y);
		emissionBuf.resize(size.x * size.y);

		shininessBuf.resize(size.x * size.y);
		depthBuf.resize(size.x * size.y);
		matTypeBuf.resize(size.x * size.y);
	}

	const GBufferElement& getAt(const glm::vec<2, int>& coords) const{
		return GBufferElement{
			wSpacePositionBuf[coords.y * size.x + coords.x],
			wSpaceNormalBuf[coords.y * size.x + coords.x],

			diffuseColorBuf[coords.y * size.x + coords.x],
			specularColorBuf[coords.y * size.x + coords.x],
			emissionBuf[coords.y * size.x + coords.x],

			shininessBuf[coords.y * size.x + coords.x],
			depthBuf[coords.y * size.x + coords.x],
			matTypeBuf[coords.y * size.x + coords.x]
		};
	}

	void setAt(const glm::vec<2, int>& coords, const GBufferElement& element) {
		wSpacePositionBuf[coords.y * size.x + coords.x] = element.worldSpacePos;
		wSpaceNormalBuf[coords.y * size.x + coords.x] = element.worldSpaceNormal;

		diffuseColorBuf[coords.y * size.x + coords.x] = element.diffuseColor;
		specularColorBuf[coords.y * size.x + coords.x] = element.specularColor;
		emissionBuf[coords.y * size.x + coords.x] = element.emission;

		shininessBuf[coords.y * size.x + coords.x] = element.shininess;
		depthBuf[coords.y * size.x + coords.x] = element.depth;
		matTypeBuf[coords.y * size.x + coords.x] = element.materialType;
	}

	void setDataFrom(const GBuffer& copyFrom){
		memcpy(wSpacePositionBuf.data(), copyFrom.wSpacePositionBuf.data(), wSpacePositionBuf.size() * sizeof(glm::vec3));
		memcpy(wSpaceNormalBuf.data(), copyFrom.wSpaceNormalBuf.data(), wSpacePositionBuf.size() * sizeof(glm::vec3));

		memcpy(diffuseColorBuf.data(), copyFrom.diffuseColorBuf.data(), wSpacePositionBuf.size() * sizeof(glm::vec3));
		memcpy(specularColorBuf.data(), copyFrom.specularColorBuf.data(), wSpacePositionBuf.size() * sizeof(glm::vec3));
		memcpy(emissionBuf.data(), copyFrom.emissionBuf.data(), wSpacePositionBuf.size() * sizeof(glm::vec3));

		memcpy(shininessBuf.data(), copyFrom.shininessBuf.data(), wSpacePositionBuf.size() * sizeof(float));
		memcpy(depthBuf.data(), copyFrom.depthBuf.data(), wSpacePositionBuf.size() * sizeof(float));
		memcpy(matTypeBuf.data(), copyFrom.matTypeBuf.data(), wSpacePositionBuf.size() * sizeof(MaterialType));


		cameraPosWS = copyFrom.cameraPosWS;
		viewMat = copyFrom.viewMat;
		invViewMat = copyFrom.invViewMat;
		focalLength = copyFrom.focalLength;
	}

	const glm::vec3& getCameraPos() const{
		return cameraPosWS;
	}

	void setCameraPos(const glm::vec3& newPos){
		cameraPosWS = newPos;
	}

	void setViewMat(const glm::mat4& newMatWC){
		viewMat = newMatWC;
	}

	void setFocalLength(float newfocalLength) {
		focalLength = newfocalLength;
	}

	void setInvViewMat(const glm::mat4& newMatCW) {
		invViewMat = newMatCW;
	}

	const glm::mat4& getViewMat() const{
		return viewMat;
	}

	const glm::mat4& getInvViewMat() const {
		return invViewMat;
	}
	float getFocalLength() const{
		return focalLength;
	}


public:
	glm::vec<2, int> size{};
	//std::vector<GBufferElement> buffer{};

	std::vector<glm::vec3> wSpacePositionBuf{};
	std::vector<glm::vec3> wSpaceNormalBuf{};
	std::vector<glm::vec3> diffuseColorBuf{};
	std::vector<glm::vec3> specularColorBuf{};
	std::vector<glm::vec3> emissionBuf{};
	std::vector<float> shininessBuf{};
	std::vector<float> depthBuf{};
	std::vector<MaterialType> matTypeBuf{};

	glm::vec3 cameraPosWS{};
	glm::mat4 viewMat{};
	glm::mat4 invViewMat{};
	float focalLength{};
};