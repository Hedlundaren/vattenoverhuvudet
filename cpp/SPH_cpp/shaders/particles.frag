#version 330 core

in vec3 normal;
in vec3 in_color;

out vec4 color;

void main() {
    color = vec4(in_color, 1.0f);
}