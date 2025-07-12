// NOTE: This file is used in both GLSL and C++
#ifndef GLOW_PIPELINE_GLOBALS_HH_
#define GLOW_PIPELINE_GLOBALS_HH_

// GLSL: pass/ao/sao/SAO.glsl
// C++: DepthPreStage.hh
#define GLOW_PIPELINE_LINEAR_DEPTH_MAX_MIP_LEVEL 5

// GLSL: pass/shadow/shadowCascades.gsh
// C++: ShadowStage.hh
#define GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT 4

// GLSL: opaque stage shaders
// C++: ShadowStage.hh
#define GLOW_PIPELINE_SHADOW_MAP_SIZE 2048

// GLSL: all clustering shaders in internal/pass/depthPre
// C++: StageCamera.cc, DepthPreStage.cc
#define GLOW_PIPELINE_CLUSTER_PIXEL_SIZE_X 120
#define GLOW_PIPELINE_CLUSTER_PIXEL_SIZE_Y 120
#define GLOW_PIPELINE_CLUSTER_AMOUNT_Z 24
#define GLOW_PIPELINE_MAX_LIGHTS_PER_CLUSTER 150

// The local workgroup size of the light assignment compute shader
// GLSL: pass/depthPre/clusterLightAssignment.csh
// C++: StageCamera.cc
#define GLOW_PIPELINE_LASSIGN_COMPUTE_X 4
#define GLOW_PIPELINE_LASSIGN_COMPUTE_Y 3

#define GLOW_PIPELINE_ENABLE_REVERSE_Z 1

#if GLOW_PIPELINE_ENABLE_REVERSE_Z
#define GLOW_PIPELINE_HORIZON_DEPTH 0.f
#else
#define GLOW_PIPELINE_HORIZON_DEPTH 1.f
#endif

#define GLOW_GLSL_FLT_MAX 3.402823466e+38

#endif
