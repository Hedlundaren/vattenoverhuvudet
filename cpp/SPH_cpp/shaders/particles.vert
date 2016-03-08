#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 velocity;

uniform mat4 P;
uniform mat4 MV;
uniform vec2 screenSize;

out vec3 vPosition;
out float vRadius;
out float vVelocity;

void main() {

    vec4 pos4 = vec4(position, 1.0f) * MV; //Fel ordning?
    vPosition = vec3(pos4);
    vRadius = 10.0f / (-vPosition.z *4.0f * (1.0f/ screenSize.y));
    vec4 clipspacePos = pos4 * P; //Fel ordning?

    gl_Position = clipspacePos;
    gl_PointSize = vRadius;

    vVelocity = length(velocity);

}

