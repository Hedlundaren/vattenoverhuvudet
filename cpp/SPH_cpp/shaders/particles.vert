#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 velocity;

uniform mat4 P;
uniform mat4 MV;
uniform vec2 screenSize;
uniform vec4 camPos;

out vec3 vPosition;
out float vRadius;
out float vVelocity;
out float vDepth;


void main() {

    vPosition = position;
    //vPosition = vec3(MV * vec4(position, 1.0f));
    vRadius = 0.2f;

    //gl_Position = P * MV * vec4(position.x, -position.y, position.z, 1.0f);
    //gl_PointSize = vRadius;

    vVelocity = length(velocity);

    //Depth
    float maxPos = length(camPos)+6.3f;
    float minPos = length(camPos)-6.3f;
    vec4 dis = vec4(position, 1.0f)-camPos;
    vDepth = (length(dis)-minPos)/(maxPos-minPos);

}

