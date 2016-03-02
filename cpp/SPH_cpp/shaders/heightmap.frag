#version 330 core

in vec3 Position;
in vec3 Normal;

out vec4 color;

void main() {
    //color = vec4(0.01 * Position, 1.0);
    color = vec4(Normal.x, 0.15 * Normal.y, Normal.z, 1.0);
}