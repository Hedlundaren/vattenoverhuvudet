#pragma once

#define GLFW_INCLUDE_GLCOREARB
#include "GLFW/glfw3.h"


#include <string>
#include <vector>
#include <iostream>

/// @brief //todo add brief description to ShaderProgram
/// @author benjamin
/// Adapted from http://stackoverflow.com/questions/2795044/easy-framework-for-opengl-shaders-in-c-c
class ShaderProgram {
public:
    /// Constructs a GLSL shader program with V/F/G-shaders located in the specified files
    ShaderProgram(std::string vertex_shader_filename,
                  std::string fragment_shader_filename,
                  std::string geometry_shader_filename = "");

    /// Get the GLuint corresponding to the OpenGL shader program
    inline operator GLuint() {
        return prog;
    }

    /// Activate the shader program
    inline void operator()() {
        glUseProgram(prog);
    }

    static const std::string getShaderType(GLuint type);

    ~ShaderProgram();

protected:
    GLuint AttachShader(GLuint shaderType, std::string source);

    void ConfigureShaderProgram();

private:
    std::vector<GLuint> shader_programs_;
    GLuint prog;

    GLuint compile(GLuint type, GLchar const *source);
};