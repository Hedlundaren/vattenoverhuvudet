#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 Position;
out vec3 Normal;

uniform mat4 MV;
uniform mat4 P;

void main() {
    gl_Position = P * MV * vec4(position.x - 14.0, position.y, position.z - 14.0, 1.0);

    Position = position;
    Normal = normal;
}