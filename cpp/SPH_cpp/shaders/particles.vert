//#version 400 compatibility
#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 velocity;

//out vec3 vPosition;
//out float vRadius;

uniform float radius;

void main() {

    //GeomShader
    gl_Position = vec4(position, 1.0f);
    gl_PointSize = 1;

    /*
    //TessShader
    vPosition = position.xyz; //Spheres
    vRadius = radius;

    //gl_Position = vec4(0, 0, 0, 1); //From tutorial of whole sphere-subdivision...
    */
}

