#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 velocity;

uniform mat4 MVP;

void main() {
    //gl_Position = MVP * vec4(position, 1.0);
    gl_Position = vec4(position, 1.0);
    gl_PointSize = 1;
}