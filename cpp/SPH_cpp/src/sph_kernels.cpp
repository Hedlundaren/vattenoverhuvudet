#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

#include "sph_kernels.h"

float normVector(std::vector<float> v) {
    float sum = 0.0f;

    //c++11
    for (float f : v) {
        sum += f * f;
    }

    return sqrt(sum);
}

float Wpoly6(std::vector<float> r, float h) {
    float w = 0.0f;
    float radius = normVector(r);

    if (radius < h && radius >= 0)
        w = (315 / (64 * M_PI * std::pow(h, 9))) * std::pow((std::pow(h, 2) - std::pow(radius, 2)), 3);
    else
        w = 0;

    return w;
}

std::vector<float> gradWpoly6(std::vector<float> r, float h) {
	float radius = normVector(r);
	std::vector<float> gradient; 

	if (radius < h && radius > 0) {
		float constant = -((315/(64*M_PI*pow(h, 9))) * 6 * pow(pow(h, 2) - pow(radius, 2), 2));
		gradient.push_back(constant * r[0]);
		gradient.push_back(constant * r[1]);
		gradient.push_back(constant * r[2]);
	} else {
		gradient.push_back(0);
		gradient.push_back(0);
		gradient.push_back(0);
	}

	return gradient;
}

float laplacianWpoly6(std::vector<float> r, float h) {
	float radius = normVector(r);
	float laplacian = 0;

	if (radius < h && radius > 0) {
		laplacian = (315/(64*M_PI*pow(h, 9))) * (24 * pow(radius, 2) * (pow(h, 2) - pow(radius, 2)) - 6 * pow(pow(h, 2) - pow(radius, 2), 2));
	} else {
		laplacian = 0;
	}

	return laplacian;
}

std::vector<float> gradWspiky(std::vector<float> r, float h) {
	std::vector<float> gradient; 
	float radius = normVector(r);
	float neg_r[3];

	for (int i = 0; i < 3; ++i) {
		neg_r[i] = -r[i];
	}

	//negative r divided by radius
	//this should be improved
	float neg_r_by_radius[3];
	for (int i = 0; i < 3; ++i) {
		neg_r_by_radius[i] = -r[i] / radius;
	}

	if (radius < h && radius > 0) {
		float constant = (15 / (M_PI * std::pow(h, 6)) * 3 * std::pow((h - radius), 2)); 
		gradient.push_back(constant * neg_r_by_radius[0]);
		gradient.push_back(constant * neg_r_by_radius[1]);
		gradient.push_back(constant * neg_r_by_radius[2]);
	}

	else {
		gradient.push_back(0);
		gradient.push_back(0);
		gradient.push_back(0);
	}

	return gradient;
}

float laplacianWviscosity(std::vector<float> r, float h) {
	float radius = normVector(r);
	float laplacian;

	std::cout << "radius = " << radius << std::endl;

	if (radius < h && radius > 0) {
		laplacian = (45/(M_PI * pow(h, 6))) * (h - radius);
	} else {
		laplacian = 0;
	}

	return laplacian;
}
