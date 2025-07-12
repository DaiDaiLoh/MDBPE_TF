#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <glow/common/shared.hh>
#include <glow/fwd.hh>

#include "../fwd.hh"
#include "StageCamera.hh"

namespace glow
{
namespace pipeline
{
enum class StageType
{
    // Internal stages do not call rc.onRenderStage
    Internal,
    DepthPre,
    Shadow,
    Opaque,
    Transparent
};

GLOW_SHARED(class, RenderStage);
class RenderStage
{
private:
    /// Whether the stage has been executed this frame
    bool mRenderedThisFrame = false;
    /// All RenderStages this stage depends upon
    std::vector<SharedRenderStage> mDependencies;
    /// Shaders that have already been registered
    /// Only used within prepareShader
    mutable std::unordered_set<Program*> mRegisteredShaders;

protected:
    /// Registers a RenderStage as a dependency of this stage
    void registerDependency(SharedRenderStage const& stage);

protected:
    /// Called when the stage is supposed to run
    virtual void onExecuteStage(RenderContext const&, RenderCallback&) = 0;
    /// Called when a new frame begins
    virtual void onBeginNewFrame(RenderPipeline&) {}

public:
    /// Executes all dependency stages, then executes this stage (if necessary)
    void execute(StageCamera const& cam, RenderPipeline& pipeline, RenderScene& scene, RenderCallback& rc);

    /// To be called when a new frame begins
    void beginNewFrame(RenderPipeline& pipeline);

    /// Returns the fixed type of this stage
    virtual StageType getType() const = 0;

    /// Registers a stage-specific shader, for example to link SSBOs and UBOs, only has to occur once
    virtual void registerShader(RenderContext const& ctx, SharedProgram const& shader) const;
    /// Uploads all stage-exclusive data, occurs each frame
    virtual void uploadShaderData(RenderContext const&, UsedProgram&) const {}

    /// Convenience all-in-one for registerShader and uploadShaderData
    /// Overhead of one unordered_set::find per call
    /// Registers the shader if not already registered, then calls uploadsShaderData
    void prepareShader(RenderContext const& ctx, SharedProgram const& shader, UsedProgram& usedShader) const;

    virtual ~RenderStage() = default;

    virtual std::string name() const = 0;
};

}
}
