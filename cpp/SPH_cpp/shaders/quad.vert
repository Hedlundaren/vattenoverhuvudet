#version 330 core

layout (location = 0) in vec3 vertex;

uniform mat4 MV;
uniform mat4 P;

out vec2 coords;


void main() {
	coords = (vertex.xy + 1.0f) / 2.0f; //coords becomes between [0 0] & [1 1]
	gl_Position = vec4(vertex, 1.0f);

}
