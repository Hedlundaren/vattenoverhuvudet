#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 Position;
out vec3 Normal;
out vec4 ProjPos;

uniform mat4 MV;
uniform mat4 P;

void main() {
    ProjPos = P * MV * vec4(position, 1.0);
    gl_Position = ProjPos;

    Position = position;
    Normal = normal;
}