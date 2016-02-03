#ifndef SPH_KERNELS_H
#define SPH_KERNELS_H

#include <iostream>
#include <vector>

// Smoothing kernel
// Used for most common calculations (e.g. density and surface tension)
float Wpoly6(glm::vec3 r, float h);

// Gradient of Wpoly6
// Used for surface normal (n)
glm::vec3 gradWpoly6(glm::vec3 r, float h);

// Laplacian of Wpoly6
// Used for curvatore of surface (k(cs))
float laplacianWpoly6(glm::vec3 r, float h);

// Gradient of Wspiky
// Used for pressure force
glm::vec3 gradWspiky(glm::vec3 r, float h);

// Laplacian of Wviscosity
// Used for Viscosity force
float laplacianWviscosity(glm::vec3 r, float h);

#endif
