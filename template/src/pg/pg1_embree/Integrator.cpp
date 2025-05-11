#include "stdafx.h"
#include "Integrator.h"

#include <iostream>

bool Integrator::sanitize(glm::vec3& l, bool logToConsole)
{
	bool changed{ false };
	if (std::isnan(l.r) || std::isnan(l.g) || std::isnan(l.b)){
		l = glm::vec3{ 0 };
		changed = true;

		if (logToConsole) std::cout << "nan" << std::endl;
	}

	if (l.r < 0 || l.g < 0 || l.b < 0) {
		l = glm::vec3{ 0 };
		changed = true;

		if (logToConsole) std::cout << "< 0" << std::endl;
	}
	return changed;
}
