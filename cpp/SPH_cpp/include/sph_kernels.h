#ifndef SPH_KERNELS_H
#define SPH_KERNELS_H

#include <iostream>
#include <vector>

float normVector(std::vector<float> v);

// Smoothing kernel
// Used for most common calculations (e.g. density and surface tension)
float Wpoly6(std::vector<float> r, float h);

// Gradient of Wpoly6
// Used for surface normal (n)
std::vector<float> gradWpoly6(std::vector<float> r, float h); 

// Laplacian of Wpoly6
// Used for curvatore of surface (k(cs))
float laplacianWpoly6(std::vector<float> r, float h); 

// Gradient of Wspiky
// Used for pressure force
std::vector<float> gradWspiky(std::vector<float> r, float h); 

#endif
