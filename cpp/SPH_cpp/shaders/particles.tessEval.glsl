#version 400 compatibility
#extension GL_ARB_tessellation_shader : enable

//Specifies if you draw triangles, quads or isolines and how the specing should be
//And if defines the object clock-wise or counter-clock-wise
layout(quads, equal_spacing, ccw) in;

patch in vec3 tcPosition;
patch in float tcRadius;
patch in float tcDepth;

out vec3 teNormal;
out float teDepth;

uniform mat4 MV;
uniform mat4 P;

const float PI = 3.14159265;

void main()
{
    vec3 p = gl_in[0].gl_Position.xyz; //vrf?

    float u = gl_TessCoord.x; //Value between [0 1]
    float v = gl_TessCoord.y;
    float w = gl_TessCoord.z; //is ignored for quads (but used for triangles)

    //Tutorials solution
    float phi = PI * (u-0.5 ); //-pi/2<phi<pi/2
    float theta = 2 * PI * (v-0.5); //-pi<theta<pi
    vec3 xyz = vec3( cos(phi)*cos(theta), sin(phi), cos(phi)*sin(theta) );

    teNormal = xyz;
    xyz *= tcRadius;
    xyz -= tcPosition;
    gl_Position = P * MV * vec4( xyz, 1);

    teDepth = tcDepth;

}

