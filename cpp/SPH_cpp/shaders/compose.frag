#version 330 core

// Parameters from the vertex shader
in vec2 coords;

// Textures
uniform sampler2D skymapTexture;
uniform sampler2D heightmapTexture;
uniform sampler2D particleTexture;

// Uniforms
uniform mat4 MV;

// Output
out vec4 outColor;

vec2 spheremap(vec3 dir) {
	float m = 2.0f * sqrt(dir.x * dir.x + dir.y * dir.y + (dir.z + 1.0f) * (dir.z + 1.0f));
	return vec2(dir.x / m + 0.5f, dir.y / m + 0.5f);
}

void main() {

    float light = 10.0f;
	vec3 dir = normalize(vec3((coords.xy - vec2(0.5f)) * 2.0f, -1.0f));

	dir = mat3(MV) * dir;
	vec2 mapCoords = spheremap(dir);

	vec4 heightmapColor = texture(heightmapTexture, coords);
	vec4 skymapColor = texture(skymapTexture, mapCoords);
	vec4 particleColor = texture(particleTexture, coords);

	float blend = (heightmapColor.a == 1.0f) ? 0.0f : 1.0f;
	vec4 backgroundColor = blend * heightmapColor + (1.0f - blend) * skymapColor;

	outColor = light * particleColor * particleColor.w + backgroundColor * (1.0f - particleColor.w);

	//Depth
	//outColor = vec4(vec3(particleColor.x * particleColor.w), 1.0f);


}
