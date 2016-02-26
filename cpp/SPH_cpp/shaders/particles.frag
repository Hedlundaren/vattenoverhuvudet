//#version 400 compatibility
#version 330 core

//TessShader
//in vec3 teNormal;

//GeomShader
in vec3 normal;
in vec3 in_color;

out vec4 color;

uniform vec3 lDir;
uniform mat4 MV;

void main() {

//GeomShader
color = vec4(in_color, 1.0f);


/*
//TessShader
vec3 V = vec3( 0.0, 0.0, 1.0 );
vec3 L = normalize(lDir);
vec3 N = normalize(teNormal);
float n = 10;
vec3 ambient = vec3( 0, 0, 0.5);
vec3 diffuse = vec3( 0, 0, 0.7);
vec3 specular = vec3( 0, 0, 1);

vec3 R = reflect(L,N);
float dotNL = max(dot(N, L), 0.0);
float dotRV = max(dot(R, V), 0.0);
if ( dotNL == 0.0 ) dotRV = 0.0; // Do not show highlight on the dark side
vec3 shadedcolor = ambient + diffuse*dotNL + specular*pow(dotRV, n);
color =  vec4( shadedcolor , 1.0 ) ;
*/
}