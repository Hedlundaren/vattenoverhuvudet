#version 330

/*
//Spheres
uniform mat4 MV;
uniform mat3 NormalMatrix;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec3 tePosition[3];
in vec3 tePatchDistance[3];
out vec3 gFacetNormal;
out vec3 gPatchDistance;
out vec3 gTriDistance;

void main()
{
    vec3 A = tePosition[2] - tePosition[0];
    vec3 B = tePosition[1] - tePosition[0];
    gFacetNormal = NormalMatrix * normalize(cross(A, B));

    gPatchDistance = tePatchDistance[0];
    gTriDistance = vec3(1, 0, 0);
    gl_Position = gl_in[0].gl_Position; EmitVertex();

    gPatchDistance = tePatchDistance[1];
    gTriDistance = vec3(0, 1, 0);
    gl_Position = gl_in[1].gl_Position; EmitVertex();

    gPatchDistance = tePatchDistance[2];
    gTriDistance = vec3(0, 0, 1);
    gl_Position = gl_in[2].gl_Position; EmitVertex();

    EndPrimitive();
}
*/

//Triangles
layout (points) in;
layout (triangle_strip, max_vertices = 12) out;
out vec3 normal;
out vec3 in_color;

uniform mat4 MV;
uniform mat4 P;

const float triangle_size = 0.02;

const vec3 RED = vec3(1.0, 0.0, 0.0);
const vec3 GREEN = vec3(0.0, 1.0, 0.0);
const vec3 BLUE = vec3(0.0, 0.0, 1.0);
const vec3 GREY = vec3(0.5, 0.5, 0.5);

void make_face(vec3 a, vec3 b, vec3 c, vec3 face_color) {
    vec3 face_normal = normalize(cross(c - a, c - b));

    normal = face_normal;
    in_color = face_color;
    gl_Position = P * MV * vec4(a, 1.0);
    EmitVertex();

    normal = face_normal;
    in_color = face_color;
    gl_Position = P * MV * vec4(b, 1.0);
    EmitVertex();

    normal = face_normal;
    in_color = face_color;
    gl_Position = P * MV * vec4(c, 1.0);
    EmitVertex();
}

void make_tetrahedron(vec3 center, float radius) {
    vec3 a = radius * normalize(vec3(1, 1, 1)) - center;
    vec3 b = radius * normalize(vec3(1, -1, -1)) - center;
    vec3 c = radius * normalize(vec3(-1, 1, -1)) - center;
    vec3 d = radius * normalize(vec3(-1, -1, 1)) - center;

    make_face(a, b, c, RED);
    make_face(a, c, d, GREEN);
    make_face(a, d, b, BLUE);
    make_face(b, d, c, GREY);

    EndPrimitive();
}

void main() {
    make_tetrahedron(gl_in[0].gl_Position.xyz, triangle_size);
}
