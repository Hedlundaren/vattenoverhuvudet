#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 velocity;

uniform mat4 MV;
uniform mat4 P;

out vec2 coords;

void main() {
	//coords = (position.xy + 1.0f) / 2.0f;
	coords = vec2(MV * vec4(position, 1.0f));
    gl_Position = P * MV * vec4(position, 1.0f);
}
