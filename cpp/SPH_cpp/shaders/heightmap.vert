#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 Normal;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(position, 1.0);
    Normal = normal;
}