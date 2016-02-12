#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 velocity;

//out vec3 vPosition;

uniform mat4 MV;
uniform mat4 P;

void main() {
    //gl_Position = P * MV * vec4(position, 1.0);
    gl_Position = vec4(position, 1.0f);//Triangles
    //vPosition = position.xyz; //Spheres
    gl_PointSize = 1;
}

