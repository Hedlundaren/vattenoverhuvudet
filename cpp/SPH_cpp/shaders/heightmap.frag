#version 330 core

in vec3 Position;
in vec3 Normal;

out vec4 color;

#define LIGHT_DIR vec3(0.0, 1.0, 0.0)

#define KA vec3(0.1, 0.1, 0.1)
#define KD vec3(0.5, 0.5, 0.5)
#define KS vec3(0.8, 0.8, 0.5)

vec3 applyPhongShading(in vec3 pos, in vec3 gradient, in vec3 light_dir, in vec3 ka, in vec3 kd, in vec3 ks) {
    vec3 normal = normalize(gradient);
    vec3 light_dir_reflected = normalize(reflect(light_dir, normal));
    //const vec3 eyeDirection = normalize(cameraPosition_ - pos);

    // Implement phong shading
    vec3 ambient = ka;
    vec3 diffuse = clamp(kd * max(0.0, dot(normal, light_dir)), 0.0, 1.0);
    //const vec3 specular = clamp(ks * pow(max(0.0, dot(-light_dir_reflected, eyeDirection)), 0.25 * shininess_), 0.0, 1.0);
    vec3 specular = vec3(0.0, 0.0, 0.0);

    return ambient + diffuse + specular;
}

void main() {
    color = vec4(applyPhongShading(Position, Normal, LIGHT_DIR, KA, KD, KS), 1.0);
}