#extension GL_EXT_gpu_shader4 : require
#extension GL_ARB_gpu_shader5 : enable
#include <glow-pipeline/internal/pass/ao/sao/reconstructCS.glsl>
#include <glow-pipeline/internal/common/globals.hh>
/**         

    SCALABLE AMBIENT OBSCURANCE


    Technique and implementation from:
        http://research.nvidia.com/sites/default/files/pubs/2012-06_Scalable-Ambient-Obscurance/McGuire12SAO.pdf
    modified code found in files:
        SAO.glsl, aoBlur.fsh, aoCSZ.fsh, aoCSZMinify.fsh, cszReconstruct.glsl

    Original license below
*/

/**
 author: Morgan McGuire and Michael Mara, NVIDIA Research

 Reference implementation of the Scalable Ambient Obscurance (SAO) screen-space ambient obscurance algorithm. 
 
 The optimized algorithmic structure of SAO was published in McGuire, Mara, and Luebke, Scalable Ambient Obscurance,
 <i>HPG</i> 2012, and was developed at NVIDIA with support from Louis Bavoil.

 The mathematical ideas of AlchemyAO were first described in McGuire, Osman, Bukowski, and Hennessy, The 
 Alchemy Screen-Space Ambient Obscurance Algorithm, <i>HPG</i> 2011 and were developed at 
 Vicarious Visions.  
 
 DX11 HLSL port by Leonardo Zide of Treyarch

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

// Total number of direct samples to take at each pixel
// Paper value: 11
#define NUM_SAMPLES 11

// If using depth mip levels, the log of the maximum pixel offset before we need to switch to a lower 
// miplevel to maintain reasonable spatial locality in the cache
// If this number is too small (< 3), too many taps will land in the same pixel, and we'll get bad variance that manifests as flashing.
// If it is too high (> 5), we'll get bad performance because we're not using the MIP levels effectively
#define LOG_MAX_OFFSET 3

// Used for preventing AO computation on the sky (at infinite depth) and defining the CS Z to bilateral depth key scaling. 
// Does not have to match the real far plane
// Paper value: -300 / 300
#define FAR_PLANE_Z 1000.0

// The number of turns around the circle that the spiral pattern makes.  Should be prime to prevent
// taps from lining up. Paper value: 7, tuned for NUM_SAMPLES = 9
#define NUM_SPIRAL_TURNS 7

uniform float           uProjScale;
uniform sampler2D       uCSZBuffer;
uniform float           uRadius;
uniform float           uBias;
uniform float           uIntensityDivR6; // intensity / radius^6


/** Returns a unit vector and a screen-space radius for the tap on a unit disk (the caller should scale by the actual disk radius) */
vec2 tapLocation(int sampleNumber, float spinAngle, out float ssR){
    // Radius relative to ssR
    float alpha = float(sampleNumber + 0.5) * (1.0 / NUM_SAMPLES);
    float angle = alpha * (NUM_SPIRAL_TURNS * 6.28) + spinAngle;

    ssR = alpha;
    return vec2(cos(angle), sin(angle));
}


/** Used for packing Z into the GB channels */
float CSZToKey(float z) {
    return clamp(z * (1.0 / FAR_PLANE_Z), 0.0, 1.0);
}


/** Used for packing Z into the GB channels */
void packKey(float key, out vec2 p) {
    // Round to the nearest 1/256.0
    float temp = floor(key * 256.0);

    // Integer part
    p.x = temp * (1.0 / 256.0);

    // Fractional part
    p.y = key * 256.0 - temp;
}

 
/** Read the camera-space position of the point at screen-space pixel ssP */
vec3 getPosition(ivec2 ssP) {
    vec3 P;
    P.z = texelFetch(uCSZBuffer, ssP, 0).r;

    // Offset to pixel center
    P = reconstructCSPosition(vec2(ssP) + vec2(0.5), P.z);
    return P;
}


/** Read the camera-space position of the point at screen-space pixel ssP + unitOffset * ssR.  Assumes length(unitOffset) == 1 */
vec3 getOffsetPosition(ivec2 ssC, vec2 unitOffset, float ssR) {
    // Derivation:
    //  mipLevel = floor(log(ssR / MAX_OFFSET));
#   ifdef GL_EXT_gpu_shader5
        int mipLevel = clamp(findMSB(int(ssR)) - LOG_MAX_OFFSET, 0, GLOW_PIPELINE_LINEAR_DEPTH_MAX_MIP_LEVEL);
#   else
        int mipLevel = clamp(int(floor(log2(ssR))) - LOG_MAX_OFFSET, 0, GLOW_PIPELINE_LINEAR_DEPTH_MAX_MIP_LEVEL);
#   endif

    ivec2 ssP = ivec2(ssR * unitOffset) + ssC;
    
    vec3 P;

    // We need to divide by 2^mipLevel to read the appropriately scaled coordinate from a MIP-map.  
    // Manually clamp to the texture size because texelFetch bypasses the texture unit
    ivec2 mipP = clamp(ssP >> mipLevel, ivec2(0), textureSize(uCSZBuffer, mipLevel) - ivec2(1));
    P.z = texelFetch(uCSZBuffer, mipP, mipLevel).r;

    // Offset to pixel center
    P = reconstructCSPosition(vec2(ssP) + vec2(0.5), P.z);

    return P;
}


float radius2 = uRadius * uRadius;

/** Compute the occlusion due to sample with index \a i about the pixel at \a ssC that corresponds
    to camera-space point \a C with unit normal \a n_C, using maximum screen-space sampling uRadius \a ssDiskRadius 

    Note that units of H() in the HPG12 paper are meters, not
    unitless.  The whole falloff/sampling function is therefore
    unitless.  In this implementation, we factor out (9 / uRadius).

    Four versions of the falloff function are implemented below
*/
float sampleAO(in ivec2 ssC, in vec3 C, in vec3 n_C, in float ssDiskRadius, in int tapIndex, in float randomPatternRotationAngle) {
    // Offset on the unit disk, spun for this pixel
    float ssR;
    vec2 unitOffset = tapLocation(tapIndex, randomPatternRotationAngle, ssR);
    ssR *= ssDiskRadius;
        
    // The occluding point in camera space
    vec3 Q = getOffsetPosition(ssC, unitOffset, ssR);

    vec3 v = Q - C;

    float vv = dot(v, v);
    float vn = dot(v, n_C);

    const float epsilon = 0.01;
    
    // A: From the HPG12 paper
    // Note large epsilon to avoid overdarkening within cracks
    // return float(vv < radius2) * max((vn - uBias) / (epsilon + vv), 0.0) * radius2 * 0.6;

    // B: Smoother transition to zero (lowers contrast, smoothing out corners). [Recommended]
    float f = max(radius2 - vv, 0.0); return f * f * f * max((vn - uBias) / (epsilon + vv), 0.0);

    // C: Medium contrast (which looks better at high radii), no division.  Note that the 
    // contribution still falls off with uRadius^2, but we've adjusted the rate in a way that is
    // more computationally efficient and happens to be aesthetically pleasing.
    // return 4.0 * max(1.0 - vv * invRadius2, 0.0) * max(vn - uBias, 0.0);

    // D: Low contrast, no division operation
    // return 2.0 * float(vv < uRadius * uRadius) * max(vn - uBias, 0.0);
}

vec3 scalableAmbientObscurance() {

    // Pixel being shaded 
    ivec2 ssC = ivec2(gl_FragCoord.xy);

    // World space point being shaded
    vec3 C = getPosition(ssC);

    vec2 key = vec2(0);
    packKey(CSZToKey(C.z), key);
    
    // Unneccessary with depth test.
    if (C.z > FAR_PLANE_Z) {
        // We're on the skybox
        discard;
    }

    // Hash function used in the HPG12 AlchemyAO paper
    float randomPatternRotationAngle = (3 * ssC.x ^ ssC.y + ssC.x * ssC.y) * 10;

    // Reconstruct normals from positions. These will lead to 1-pixel black lines
    // at depth discontinuities, however the blur will wipe those out so they are not visible
    // in the final image.
    vec3 n_C = reconstructCSFaceNormal(C);
    
    // Choose the screen-space sample radius
    // proportional to the projected area of the sphere
    float ssDiskRadius = -uProjScale * uRadius / C.z;
    
    float sum = 0.0;
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        sum += sampleAO(ssC, C, n_C, ssDiskRadius, i, randomPatternRotationAngle);
    }

    float A = max(0.0, 1.0 - sum * uIntensityDivR6 * (5.0 / NUM_SAMPLES));

    // Bilateral box-filter over a quad for free, respecting depth edges
    // (the difference that this makes is subtle)
    if (abs(dFdx(C.z)) < 0.02) {
        A -= dFdx(A) * ((ssC.x & 1) - 0.5);
    }
    if (abs(dFdy(C.z)) < 0.02) {
        A -= dFdy(A) * ((ssC.y & 1) - 0.5);
    }

    return vec3(A, key);
}
