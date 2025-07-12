#pragma once

#include <functional>

#include <clean-core/optional.hh>

#include <typed-geometry/tg-lean.hh>

#include <glow/fwd.hh>

#include <glow/common/property.hh>
#include <glow/common/shared.hh>
#include <glow/objects/VertexArray.hh>

#include <glow-extras/vector/fwd.hh>

#include "../aabb.hh"
#include "../fwd.hh"

#include "../picking.hh"

namespace glow
{
namespace viewer
{
struct RenderInfo;

class Renderable : public std::enable_shared_from_this<Renderable>
{
    // member
private:
    tg::mat4 mTransform = tg::mat4::identity;
    bool mInitialized = false;
    bool mIsDirty = false; // this is opt-in
    bool mCanBeEmpty = false;
    std::string mName;
    mutable cc::optional<size_t> mCachedHash;
    std::optional<Picker> mPicker;

    // properties
public:
    GLOW_BUILDER(transform, Transform);
    GLOW_BUILDER(name, Name);
    bool isDirty() const { return mIsDirty; }
    void setDirty() { mIsDirty = true; }
    void setCanBeEmpty() { mCanBeEmpty = true; }
    bool canBeEmpty() const { return mCanBeEmpty; }
    void setPicker(Picker p) { mPicker = std::move(p); }
    bool hasPicker() const { return mPicker.has_value(); }
    Picker& getPicker() { return mPicker.value(); }

    size_t queryHash() const
    {
        TG_ASSERT(mInitialized);
        if (!mCachedHash.has_value())
            mCachedHash = computeHash();
        return mCachedHash.value();
    }

    // must be called whenever something changed the renderable
    void clearHash() { mCachedHash = {}; }

    // methods
public:
    virtual void renderShadow(RenderInfo const& /*info*/) {}
    virtual void renderForward(RenderInfo const& /*info*/) {}
    virtual void renderTransparent(RenderInfo const& /*info*/) {}
    virtual void renderOverlay(RenderInfo const& /*info*/, vector::OGLRenderer const* /*oglRenderer*/, tg::isize2 const& /*res*/, tg::ipos2 const& /*offset*/)
    {
    }
    virtual void renderPicking(RenderInfo const& /*info*/, int32_t /*RenderableID*/) {}

    // NOTE: glow::viewer::interactive possibly obsoletes this feature,
    // but the performance overhead is low
    virtual void onGui() {}

    virtual void init() {}

    virtual bool isEmpty() const { return false; }

    /// computes the AABB (update min/max)
    virtual aabb computeAabb() = 0;

    void onRenderFinished();

    /// Calls virtual init once
    void runLazyInit()
    {
        if (!mInitialized)
        {
            init();
            mInitialized = true;
        }
    }

    virtual bool isNullRenderable() const { return false; }

protected:
    /// computes a hash such that different rendering will most likely produce different hash
    virtual size_t computeHash() const = 0;

    // ctor
public:
    virtual ~Renderable() {}
};

// dummy renderable used for non-specific configures
class NullRenderable : public Renderable
{
public:
    aabb computeAabb() override { return aabb::empty(); }
    size_t computeHash() const override { return 0x91241; }
    bool isNullRenderable() const override { return true; }
};
}
}
