#extension GL_EXT_gpu_shader4 : require

uniform sampler2D uInput;
uniform int uPreviousMIPLevel;

out vec2 fLinearDepth;

void main() {
    ivec2 ssP = ivec2(gl_FragCoord.xy);

    // Rotated grid subsampling to avoid XY directional bias or Z precision bias while downsampling.
    // On DX9, the bit-and can be implemented with floating-point modulo
    fLinearDepth = 
        vec2(
            texelFetch2D(uInput, clamp(ssP * 2 + ivec2(ssP.y & 1, ssP.x & 1), ivec2(0), textureSize(uInput, uPreviousMIPLevel) - ivec2(1)), uPreviousMIPLevel).r,
            0 // CoC is not properly downsampled
        );
}
