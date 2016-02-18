#version 400 compatibility

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 velocity;

out vec3 vPosition;
out float vRadius;

uniform float radius;

void main() {

    vPosition = position.xyz; //Spheres
    vRadius = radius;

    //gl_Position = vec4(0, 0, 0, 1); //From tutorial of whole sphere-subdivision...

}

