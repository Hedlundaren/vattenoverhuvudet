#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

#define _USE_MATH_DEFINES
#include <cmath>
#include "glm/glm.hpp"

#include "sph_kernels.h"

#include "constants.hpp"

float Wpoly6(glm::vec3 r, float h) {
    float w = 0.0f;
    float radius = glm::length(r);

    if (radius < h && radius >= 0)
        w = (315 / (64 * constants::PI * std::pow(h, 9))) * std::pow((std::pow(h, 2) - std::pow(radius, 2)), 3);
    else
        w = 0;

    return w;
}

glm::vec3 gradWpoly6(glm::vec3 r, float h) {
	float radius = glm::length(r);
	glm::vec3 gradient;

	if (radius < h && radius > 0) {
		gradient = (float) -((315/(64*constants::PI*pow(h, 9))) * 6 * pow(pow(h, 2) - pow(radius, 2), 2)) * r;
	} else {
		gradient = {0, 0, 0};
	}

	return gradient;
}

float laplacianWpoly6(glm::vec3 r, float h) {
	float radius = glm::length(r);
	float laplacian = 0;

	if (radius < h && radius > 0) {
		laplacian = (315/(64*constants::PI*pow(h, 9))) * (24 * pow(radius, 2) * (pow(h, 2) - pow(radius, 2)) - 6 * pow(pow(h, 2) - pow(radius, 2), 2));
	} else {
		laplacian = 0;
	}

	return laplacian;
}

glm::vec3 gradWspiky(glm::vec3 r, float h) {
	glm::vec3 gradient;
	float radius = glm::length(r);

	if (radius < h && radius > 0) {
		gradient = (float) ((15 / (constants::PI * std::pow(h, 6))) * 3.0f * std::pow((h - radius), 2)) * (-r / radius);
	} else {
		gradient = {0, 0, 0};
	}

	return gradient;
}

float laplacianWviscosity(glm::vec3 r, float h) {
	float radius = glm::length(r);
	float laplacian;

	if (radius < h && radius > 0) {
		laplacian = (45/(constants::PI * pow(h, 6))) * (h - radius);
	} else {
		laplacian = 0;
	}

	return laplacian;
}
