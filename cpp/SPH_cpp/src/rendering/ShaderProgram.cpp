#include "rendering/ShaderProgram.hpp"

#include <string>
#include <memory>

#include "common/FileReader.hpp"

ShaderProgram::ShaderProgram(std::string vertex_shader_filename, std::string fragment_shader_filename,
                                     std::string geometry_shader_filename) {
    auto v_source = FileReader::ReadFromFile(vertex_shader_filename);
    auto f_source = FileReader::ReadFromFile(fragment_shader_filename);

    AttachShader(GL_VERTEX_SHADER, v_source);
    AttachShader(GL_FRAGMENT_SHADER, f_source);

    if (geometry_shader_filename != "") {
        auto g_source = FileReader::ReadFromFile(geometry_shader_filename);
        AttachShader(GL_GEOMETRY_SHADER, g_source);

        std::cout << "Created vertex, geometry and fragment ShaderProgram.\n";
    } else {
        std::cout << "Created vertex and fragment ShaderProgram.\n";
    }

    ConfigureShaderProgram();
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(prog);
}

GLuint ShaderProgram::AttachShader(GLuint shaderType, std::string source) {
    GLuint sh = compile(shaderType, source.c_str());
    shader_programs_.push_back(sh);

    std::cout << "Attached shader of type: '" << getShaderType(shaderType) << "'\n";

    return sh;
}

void ShaderProgram::ConfigureShaderProgram() {
    prog = glCreateProgram();

    for (GLuint shader_program : shader_programs_) {
        glAttachShader(prog, shader_program);
    }

    glLinkProgram(prog);

    GLint isLinked = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, (int *) &isLinked);

    if (isLinked == GL_FALSE) {
        GLint length;
        glGetShaderiv(prog, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, ' ');
        glGetShaderInfoLog(prog, length, &length, &log[0]);
        std::cerr << "Failed to link shaderprogram : " << std::endl
        << log << std::endl;
        exit(EXIT_FAILURE);
    } else {
        std::cout << "Shader linking complete!\n";
    }
}

const std::string ShaderProgram::getShaderType(GLuint type) {
    std::string name;
    switch (type) {
        case GL_VERTEX_SHADER:
            name = "Vertex Shader";
            break;
        case GL_FRAGMENT_SHADER:
            name = "Fragment Shader";
            break;
        case GL_GEOMETRY_SHADER:
            name = "Geometry Shader";
            break;
        default:
            name = "Unknown Shader type";
            break;
    }
    return name;
}

GLuint ShaderProgram::compile(GLuint type, GLchar const *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, ' ');
        glGetShaderInfoLog(shader, length, &length, &log[0]);
        std::cerr << "Failed to compile shadertype: " + getShaderType(type) << std::endl
        << log << std::endl;
        exit(EXIT_FAILURE);
        return false;
    }
    return shader;
}
