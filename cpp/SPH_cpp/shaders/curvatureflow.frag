#version 330 core

// Parameters from the vertex shader
in vec2 coords;

// Textures
uniform sampler2D particleTexture;

// Uniforms
uniform vec2 screenSize;
uniform mat4 P;

// Output
out float outDepth;

// Mean curvature. From "Screen Space Fluid Rendering with Curvature Flow"
vec3 meanCurvature(vec2 pos) {
	// Width of one pixel
	vec2 dx = vec2(1.0f / screenSize.x, 0.0f);
	vec2 dy = vec2(0.0f, 1.0f / screenSize.y);

	// Central z value
	float zc =  texture(particleTexture, pos).z;

	// Take finite differences
	// Central differences give better results than one-sided here.
	// TODO better boundary conditions, possibly.
	// Remark: This is not easy, get to choose between bad oblique view smoothing
	// or merging of unrelated particles
	float zdxp = texture(particleTexture, pos + dx).z;
	float zdxn = texture(particleTexture, pos - dx).z;
	float zdx = 0.5f * (zdxp - zdxn);
	zdx = (zdxp == 0.0f || zdxn == 0.0f) ? 0.0f : zdx;

	float zdyp = texture(particleTexture, pos + dy).z;
	float zdyn = texture(particleTexture, pos - dy).z;
	float zdy = 0.5f * (zdyp - zdyn);
	zdy = (zdyp == 0.0f || zdyn == 0.0f) ? 0.0f : zdy;

	// Take second order finite differences
	float zdx2 = zdxp + zdxn - 2.0f * zc;
	float zdy2 = zdyp + zdyn - 2.0f * zc;

	// Second order finite differences, alternating variables
	float zdxpyp = texture(particleTexture, pos + dx + dy).z;
	float zdxnyn = texture(particleTexture, pos - dx - dy).z;
	float zdxpyn = texture(particleTexture, pos + dx - dy).z;
	float zdxnyp = texture(particleTexture, pos - dx + dy).z;
	float zdxy = (zdxpyp + zdxnyn - zdxpyn - zdxnyp) / 4.0f;

	// Projection transform inversion terms
	float cx = 2.0f / (screenSize.x * -P[0][0]);
	float cy = 2.0f / (screenSize.y * -P[1][1]);

	// Normalization term
	float d = cy * cy * zdx * zdx + cx * cx * zdy * zdy + cx * cx * cy * cy * zc * zc;

	// Derivatives of said term
	float ddx = cy * cy * 2.0f * zdx * zdx2 + cx * cx * 2.0f * zdy * zdxy + cx * cx * cy * cy * 2.0f * zc * zdx;
	float ddy = cy * cy * 2.0f * zdx * zdxy + cx * cx * 2.0f * zdy * zdy2 + cx * cx * cy * cy * 2.0f * zc * zdy;

	// Temporary variables to calculate mean curvature
	float ex = 0.5f * zdx * ddx - zdx2 * d;
	float ey = 0.5f * zdy * ddy - zdy2 * d;

	// Finally, mean curvature
	float h = 0.5f * ((cy * ex + cx * ey) / pow(d, 1.5f));

	return(vec3(zdx, zdy, h));
}

void main() {
	float particleDepth = texture(particleTexture, coords).z;

	if(particleDepth == 0.0f) {
		outDepth = 0.0f;
	}
	else {
		const float dt = 0.00055f;
		const float dzt = 1000.0f;
		vec3 dxyz = meanCurvature(coords);

		// Vary contribution with absolute depth differential - trick from pySPH
		outDepth = particleDepth + dxyz.z * dt * (1.0f + (abs(dxyz.x) + abs(dxyz.y)) * dzt);
	}
}