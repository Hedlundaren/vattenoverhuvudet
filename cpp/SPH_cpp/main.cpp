#include <iostream>
#include <fstream>
#include <vector>

#include "Particle.h"
#include "Parameters.h"
#include "sph_kernels.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include "math/randomized.hpp"
#include "common/stream_utils.hpp"

#include "opencl_context_info.hpp"

#include "GLFW/glfw3.h"
#include <unistd.h>

int main() {
    PrintOpenClContextInfo();

    const GLFWvidmode *vidmode;  // GLFW struct to hold information about the display
    GLFWwindow *window;    // GLFW struct to hold information about the window

    // Initialise GLFW
    glfwInit();

    // Determine the desktop size
    vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    // Make sure we are getting a GL context of at least version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // Exclude old legacy cruft from the context. We don't need it, and we don't want it.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Open a square window (aspect 1:1) to fill half the screen height
    window = glfwCreateWindow(800, 600, "FluidSPH", NULL, NULL);
    if (!window) {
        glfwTerminate(); // No window was opened, so we can't continue in any useful way
        return -1;
    }

    // Make the newly created window the "current context" for OpenGL
    // (This step is strictly required, or things will simply not work)
    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {
        // Exit if the ESC key is pressed (and also if the window is closed).
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
