#version 330 core

in vec3 vPosition;
in float vRadius;
in float vVelocity;
in float vDepth;

uniform mat4 P;
uniform vec2 screenSize;


//uniform sampler2D terrainTexture;

out float particleDepth;


void main() {

	vec3 normal;

    // See where we are inside the point sprite
    normal.xy = (gl_PointCoord - 0.5f) * 2.0f; //gl_PointCoord ranges [0,1] if frag is inside point-sprite
    float dist = length(normal); // normal is [-1, -1] - [1, 1] if inside sphere

    // Outside sphere? Discard.
    if(dist > 1.0f) {
        discard;
    }


    // Set up rest of normal
    normal.z = sqrt(1.0f - dist);
    normal.y = -normal.y; //?
    normal = normalize(normal);

    // Calculate fragment position in eye space, project to find depth
    vec4 fragPos = vec4(vPosition + normal * vRadius/screenSize.y, 1.0f);// vRadius/screenSize.y;
    vec4 clipspacePos = P * fragPos;

    // Set up output
    float far = gl_DepthRange.far;
    float near = gl_DepthRange.near;
    float deviceDepth = clipspacePos.z / clipspacePos.w;
    float fragDepth = (((far - near) * deviceDepth) + near + far) / 2.0;
    gl_FragDepth = fragDepth;

    //if(fragDepth > texture(terrainTexture, gl_FragCoord.xy / screenSize).w) {
    //    discard;
    //}
    //particleDepth = clipspacePos.z;

    particleDepth = vDepth + dist/60.0f;

}
