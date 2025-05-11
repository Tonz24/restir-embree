#pragma once
#include "triangle.h"

struct TriangleCDFSample{
	const Triangle& triangle{};
	float pdf{};
};


class TriangleCDF {
public:

	TriangleCDF() = default;

	TriangleCDF(const std::vector<Triangle*>& tris);

	//<triangle, prob>
	TriangleCDFSample getTriangle() const;

	bool isValid() const {
		return tris.size() > 0;
	}


	float getPDFForTriangle(const Triangle& t) const{
		//probability of choosing this triangle
		float pdf = t.getSurfaceArea() / totalSurface;
		//probability of choosing any point on this triangle
		pdf *= 1.0f / t.getSurfaceArea();
		return pdf;
	}

	float getTotalSurfaceArea() const; 

	std::vector<Triangle*> tris{};

private:

	float totalSurface{};

	std::vector<std::pair<float,Triangle*>> cdf{};
	std::vector<float> cdf2{};
	std::vector<Triangle*> triangles{};

	static std::mt19937 generator;
	mutable std::uniform_int_distribution<int> unifDistInt{};
};