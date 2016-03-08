#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 velocity;

out vec2 coords;

void main() {
	coords = (position.xy + 1.0f) / 2.0f;
    gl_Position = vec4(position, 1.0f);
}
