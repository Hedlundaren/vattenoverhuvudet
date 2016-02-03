#include "OpenClParticleSimulator.hpp"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.hpp>
#endif



void OpenClParticleSimulator::setupSimulation(const std::vector<glm::vec3> &particle_positions,
                                              const std::vector<glm::vec3> &particle_velocities,
                                              const GLuint &vbo_positions,
                                              const GLuint &vbo_velocities) {

}

void OpenClParticleSimulator::updateSimulation(float dt_seconds) {

}

void OpenClParticleSimulator::initOpenCL() {

}