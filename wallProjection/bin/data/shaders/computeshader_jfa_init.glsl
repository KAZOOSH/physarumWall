#version 440

// Jump Flooding Algorithm
// Reads the object mask (R channel: 1.0 = free space, 0.0 = object)
// Outputs seed coordinates: free pixels seed themselves, object pixels seed (−1,−1)
layout(rg16f, binding = 0) uniform readonly image2D objectMask;
layout(rg32f, binding = 1) uniform writeonly image2D jfaOut;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main() {
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    float isFree = imageLoad(objectMask, pix).r;

    // free space: seed = own position; object: seed = invalid (-1,-1)
    if (isFree > 0.5) {
        imageStore(jfaOut, pix, vec4(float(pix.x), float(pix.y), 0.0, 0.0));
    } else {
        imageStore(jfaOut, pix, vec4(-1.0, -1.0, 0.0, 0.0));
    }
}
