//#version 400 compatibility
#version 330 core
#define USE_TESS_SHADER

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 velocity;

//TessShader
out vec3 vPosition;
out float vRadius;
out float vDepth;

uniform float radius;
uniform vec4 camPos;

void main() {

/*
    //GeomShader
    gl_Position = vec4(position, 1.0f);
    gl_PointSize = 1;
    */
/*--------------------------------------------------*/

    //TessShader
    vPosition = position.xyz; //Spheres
    vRadius = radius;

    gl_Position = vec4(position, 1.0f);
    //gl_Position = vec4(0.0f, 1.0f, 0.0f, 1.0f);

    //Depth
    float maxPos = length(camPos)+1.5f;
    float minPos = length(camPos)-1.5f;
    vec4 dis = vec4(position, 1.0f)-camPos;
    vDepth = (length(dis)-minPos)/(maxPos-minPos);

}

