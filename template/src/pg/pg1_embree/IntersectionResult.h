#pragma once
#include "HitInfo.h"
#include "material.h"

struct IntersectionResult {
	HitInfo hitInfo{};
	Material* material{ nullptr };
};