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

    vec4 pos4 = MV * vec4(position, 1.0f); //Fel ordning?  * MV
    vPosition = vec3(pos4);
    vRadius = 10.0f / (-vPosition.z *4.0f * (1.0f/ screenSize.y));
    vec4 clipspacePos = P * pos4; //Fel ordning?  * P

    gl_Position = clipspacePos;
    gl_PointSize = vRadius;

    vVelocity = length(velocity);

}

