#pragma once

/// This header includes forward declarations for all pipeline classes

#include <glow/common/shared.hh>

namespace glow
{
namespace pipeline
{
struct RenderContext;
class RenderCallback;

GLOW_SHARED(class, RenderPipeline);
GLOW_SHARED(class, RenderScene);
GLOW_SHARED(class, RenderStage);
struct CameraData;
GLOW_SHARED(class, StageCamera);

// RenderStage Implementations
GLOW_SHARED(class, PostprocessingStage);
GLOW_SHARED(class, OutputStage);
GLOW_SHARED(class, OpaqueStage);
GLOW_SHARED(class, TemporalResolveStage);
GLOW_SHARED(class, OITStage);
GLOW_SHARED(class, LightingCombinationStage);
GLOW_SHARED(class, DepthPreStage);
GLOW_SHARED(class, ShadowStage);
GLOW_SHARED(class, AOStage);

GLOW_SHARED(struct, Light);
}
}
