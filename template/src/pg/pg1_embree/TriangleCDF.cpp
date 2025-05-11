#include "stdafx.h"
#include "TriangleCDF.h"

#include "Utils.h"

std::mt19937 TriangleCDF::generator{ 123 };

TriangleCDF::TriangleCDF(const std::vector<Triangle*>& tris): tris(tris) {

	int size = static_cast<int>(tris.size()) > 0 ? tris.size(): 10;

	unifDistInt = std::uniform_int_distribution<int>(0, size - 1);

	//calculate total area of provided triangles (normalization factor)
	for (const auto* tri : tris)
		totalSurface += tri->getSurfaceArea();

	//normalize each triangle area, accumulate cdf
	for (int i = 0; i < tris.size(); ++i){

		const float normArea = tris[i]->getSurfaceArea() / totalSurface;
		const float predecessorVal = i == 0 ? 0 : cdf[i - 1].first;

		cdf.push_back({ predecessorVal + normArea , tris[i] });
	}

	std::sort(cdf.begin(), cdf.end(), [](auto &left, auto& right){
		return left.first < right.first;
	});

	for (const auto& item : cdf){
		cdf2.push_back(item.first);
	}
}

TriangleCDFSample TriangleCDF::getTriangle() const {

	int index = unifDistInt(generator);


	float ksi = Utils::getRandomValue(0.0f, 1.0f);

	auto lower = std::lower_bound(cdf2.begin(), cdf2.end(), ksi);
	index = lower - cdf2.begin();

	if (index >= cdf2.size())
		index = cdf2.size() - 1;

	if (index == 0) return { *cdf[0].second, cdf[0].first };

	float prob = cdf[index].first - cdf[index - 1].first;

	return {*cdf[index].second,prob};
}

float TriangleCDF::getTotalSurfaceArea() const{
	return totalSurface;
}