#version 330 core

in vec3 Normal;

out vec4 color;

void main() {
    color = vec4(Normal, 1.0);
}