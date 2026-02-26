#version 440

uniform int width;
uniform int height;

layout(rg16f, binding = 0) uniform readonly image2D trailRead;
layout(rgba8, binding = 7) uniform writeonly image2D trailMap;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main() {
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);

    vec2 prevColor = imageLoad(trailRead, pix).xy;

    float trailColorValue = pow(tanh(9.0 * pow(max(0., (250 * prevColor.y - 1) / 1100.0), 0.3)), 8.5) * 1.05;
    trailColorValue = min(1.0, trailColorValue);

    vec3 col = vec3(trailColorValue);
    vec4 outputColor = vec4(col, 1.0);

    col = clamp(col, vec3(0.), vec3(1.));
    outputColor = vec4(col, 1.0);

    imageStore(trailMap, pix, outputColor);
}
