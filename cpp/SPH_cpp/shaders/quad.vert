#version 330 core

in vec4 vertex;

uniform mat4 MV;
uniform mat4 P;

out vec2 coords;

void main() {
	coords = (vertex.xy + 1.0f) / 2.0f; //coords becomes between [0 0] & [1 1]
	gl_Position = vertex;

}
