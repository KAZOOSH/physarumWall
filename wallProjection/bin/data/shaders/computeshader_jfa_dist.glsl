#version 440

// Converts final JFA seed map to a normalized distance field.
// Output R channel: 0.0 = free space (or at free pixel), 1.0 = maxDist away from free space.
uniform int width;
uniform int height;
uniform float maxDist; // normalisation range in pixels

layout(rg32f, binding = 0) uniform readonly  image2D jfaIn;
layout(rg16f, binding = 1) uniform writeonly image2D distOut;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main() {
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    vec2 seed = imageLoad(jfaIn, pix).xy;

    float dist = 0.0;
    if (seed.x >= 0.0) {
        dist = distance(vec2(pix), seed);
    }

    float normalized = clamp(dist / maxDist, 0.0, 1.0);
    imageStore(distOut, pix, vec4(normalized, 0.0, 0.0, 0.0));
}
