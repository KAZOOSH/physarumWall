#version 440

// One pass of Jump Flooding Algorithm.
// Each pixel looks at 8 neighbours at ±stepSize and keeps the closest seed.
uniform int stepSize;
uniform int width;
uniform int height;

layout(rg32f, binding = 0) uniform readonly  image2D jfaIn;
layout(rg32f, binding = 1) uniform writeonly image2D jfaOut;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main() {
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);

    vec2 bestSeed = imageLoad(jfaIn, pix).xy;
    float bestDist = (bestSeed.x < 0.0) ? 1e9 : distance(vec2(pix), bestSeed);

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            ivec2 nb = pix + ivec2(dx, dy) * stepSize;
            if (nb.x < 0 || nb.y < 0 || nb.x >= width || nb.y >= height) continue;

            vec2 seed = imageLoad(jfaIn, nb).xy;
            if (seed.x < 0.0) continue;

            float d = distance(vec2(pix), seed);
            if (d < bestDist) {
                bestDist = d;
                bestSeed = seed;
            }
        }
    }

    imageStore(jfaOut, pix, vec4(bestSeed, 0.0, 0.0));
}
