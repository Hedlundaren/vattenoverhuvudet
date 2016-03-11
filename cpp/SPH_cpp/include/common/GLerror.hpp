#pragma once

#include "GLFW/glfw3.h"

inline void CheckGLerror(std::string filename, unsigned int line) {
#ifdef MY_DEBUG
    GLenum err = GL_NO_ERROR;

    if ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error code: " << err << "in file " << filename << " on line " << line << "." << std::endl;
        std::exit(EXIT_FAILURE);
    }
#endif
}