#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 velocity;

// Same for the whole model or scene: Projection and Modelview matrices
uniform mat4 P;
uniform mat4 MV;

// To fragment shader
out vec3 objectPos;
out vec3 worldPos;

void main() {
	objectPos = position;

	// Transform the vertex according to MV
	vec4 viewVertex = MV * vec4(position, 1.0f); // * MV;
	worldPos = viewVertex.xyz;

	// Project and send to the fragment shader
	gl_Position = P * viewVertex; //* P;
}
