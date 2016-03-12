#version 330 core

// Parameters from the vertex shader
in vec2 coords;

// Textures
uniform sampler2D skymapTexture;
uniform sampler2D particleTexture;
uniform sampler2D particleThicknessTexture;
uniform sampler2D velocityTexture;

// Uniforms
uniform vec2 screenSize;
uniform mat4 P;
uniform mat4 MV;
uniform vec3 lDir;
uniform vec4 camPos;

// Output
out vec4 outColor;

vec2 spheremap(vec3 dir) {
	float m = 2.0f * sqrt(dir.x * dir.x + dir.y * dir.y + (dir.z + 1.0f) * (dir.z + 1.0f));
	return vec2(dir.x / m + 0.5f, dir.y / m + 0.5f);
}

vec3 eyespacePos(vec2 pos, float depth) {
	pos = (pos - vec2(0.5f)) * 2.0f;
	return(depth * vec3(-pos.x * P[0][0], -pos.y * P[1][1], 1.0f));
}

// Calculate fresnel coefficient
// Schlicks approximation is for lamers
float fresnel(float rr1, float rr2, vec3 n, vec3 d) {
	float r = rr1 / rr2;
	float theta1 = dot(n, -d);
	float theta2 = sqrt(1.0f - r * r * (1.0f - theta1 * theta1));

	// Figure out what the Fresnel equations say about what happens next
	float rs = (rr1 * theta1 - rr2 * theta2) / (rr1 * theta1 + rr2 * theta2);
	rs = rs * rs;
	float rp = (rr1 * theta2 - rr2 * theta1) / (rr1 * theta2 + rr2 * theta1);
	rp = rp * rp;

	return((rs + rp) / 2.0f);
}

// Compute eye-space normal. Adapted from PySPH.
vec3 eyespaceNormal(vec2 pos) {
	// Width of one pixel
	vec2 dx = vec2(1.0f / screenSize.x, 0.0f);
	vec2 dy = vec2(0.0f, 1.0f / screenSize.y);

	// Central z
	float zc =  texture(particleTexture, pos).r;

	// Derivatives of z
	// For shading, one-sided only-the-one-that-works version
	float zdxp = texture(particleTexture, pos + dx).r;
	float zdxn = texture(particleTexture, pos - dx).r;
	float zdx = (zdxp == 0.0f) ? (zdxn == 0.0f ? 0.0f : (zc - zdxn)) : (zdxp - zc);

	float zdyp = texture(particleTexture, pos + dy).r;
	float zdyn = texture(particleTexture, pos - dy).r;
	float zdy = (zdyp == 0.0f) ? (zdyn == 0.0f ? 0.0f : (zc - zdyn)) : (zdyp - zc);

	// Projection inversion
	float cx = 2.0f / (screenSize.x * -P[0][0]);
	float cy = 2.0f / (screenSize.y * -P[1][1]);

	// Screenspace coordinates
	float sx = floor(pos.x * (screenSize.x - 1.0f));
	float sy = floor(pos.y * (screenSize.y - 1.0f));
	float wx = (screenSize.x - 2.0f * sx) / (screenSize.x * P[0][0]);
	float wy = (screenSize.y - 2.0f * sy) / (screenSize.y * P[1][1]);

	// Eyespace position derivatives
	vec3 pdx = normalize(vec3(cx * zc + wx * zdx, wy * zdx, zdx));
	vec3 pdy = normalize(vec3(wx * zdy, cy * zc + wy * zdy, zdy));

	return normalize(cross(pdx, pdy));
}


void main() {
	float particleDepth = texture(particleTexture, coords).r; //Red channel used
	float particleThickness = texture(particleThicknessTexture, coords).r;
	float velocity = texture(velocityTexture, coords).r;
	vec3 inNormal = texture(particleThicknessTexture, coords).gba;

	vec3 normal = eyespaceNormal(coords);
	normal = inverse(mat3(MV)) * normal;
	normal.xz = -normal.xz;


	if(particleDepth == 0.0f) {
		outColor = vec4(0.0f);
	}
	else {

	    vec3 pos = eyespacePos(coords, particleDepth);
        pos = vec3(inverse(MV)* vec4(pos, 1.0f));

        float thickness = (particleThickness) * 20.0f;

        vec3 lightDir = normalize(lDir);
        inNormal = normalize(inNormal);
        float diffuse = max(0.0f, dot(lightDir, inNormal));

        vec3 fromEye = normalize(pos);
        fromEye.xz = -fromEye.xz;
        //vec3 fromEye = vec3(camPos);

        float alpha = 3.0f;
        vec3 R = reflect(lightDir, inNormal);
        float specular = pow(max(0.0f, dot(R,-fromEye)), alpha);
        if ( diffuse == 0.0 ) specular = 0.0;

        //specular = clamp(fresnel(1.0f, 1.5f, inNormal, fromEye), 0.0f, 0.4f);

        // De-specularize fast particles
        //specular = max(0.0f, specular - (velocity / 15.0f));

        vec4 skymapColor = texture(skymapTexture, spheremap(R));
        vec4 particleColor = exp(-vec4(0.4f, 0.1f, 0.05f, 3.0f) * thickness);
        particleColor.w = clamp(1.0f - particleColor.w, 0.0f, 1.0f);
        particleColor.rgb = (diffuse + 0.2f) * particleColor.rgb * (1.0f - specular) + specular*particleColor.rgb + specular * skymapColor.rgb;

        //outColor = vec4(inNormal, 1.0f);
        //outColor = vec4(fromEye, 1.0f);
        //outColor = vec4(particleDepth);
        outColor = particleColor;
	}
}
