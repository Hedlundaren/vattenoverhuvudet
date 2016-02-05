#version 330

layout (points) in;
layout (triangle_strip, max_vertices = 12) out;
out vec3 normal;
out vec3 in_color;

uniform mat4 MVP;

const float triangle_size = 0.01;

const vec3 RED = vec3(1.0, 0.0, 0.0);
const vec3 GREEN = vec3(0.0, 1.0, 0.0);
const vec3 BLUE = vec3(0.0, 0.0, 1.0);
const vec3 GREY = vec3(0.5, 0.5, 0.5);

void make_face(vec3 a, vec3 b, vec3 c, vec3 face_color) {
    vec3 face_normal = normalize(cross(c - a, c - b));

    normal = face_normal;
    in_color = face_color;
    gl_Position = MVP * vec4(a, 1.0);
    EmitVertex();

    normal = face_normal;
    in_color = face_color;
    gl_Position = MVP * vec4(b, 1.0);
    EmitVertex();

    normal = face_normal;
    in_color = face_color;
    gl_Position = MVP * vec4(c, 1.0);
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
