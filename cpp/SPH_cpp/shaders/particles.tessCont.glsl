#version 400 compatibility
#extension GL_ARB_tessellation_shader : enable

layout(vertices = 1) out; //variables defined per-vertex=out, per-patch=patch out. Plus number of vertices output

in vec3 vPosition[];
in float vRadius[];

patch out vec3 tcPosition;
patch out float tcRadius;

float uDetail = 50;
float TessLevelInner = 3;
float TessLevelOuter = 4;

#define ID gl_InvocationID

void main()
{
    //ID tells what vertex you are on, must be mapped to gl_out in TCS
    //gl_out[ID].gl_Position = gl_in[0].gl_Position; //(0,0,0,1)

    tcPosition = vPosition[0];
    tcRadius = vRadius[0];

    //Arrays with up to 4 resp. 2 (outer, inner) edges of tessellation levels
    gl_TessLevelOuter[0] = 4; // Outer [0],[2] are the nr.of div. at the poles
    gl_TessLevelOuter[1] = TessLevelOuter * tcRadius * uDetail; // Outer [1],[3] are the nr.of div. at vertical seams
    gl_TessLevelOuter[2] = 4;
    gl_TessLevelOuter[3] = TessLevelOuter * tcRadius * uDetail;
    gl_TessLevelInner[0] = TessLevelInner * tcRadius * uDetail; // Inner [0],[1] are the inside sphere detail
    gl_TessLevelInner[1] = TessLevelInner * tcRadius * uDetail;

}
