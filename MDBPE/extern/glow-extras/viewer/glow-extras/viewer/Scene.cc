#include "Scene.hh"

#include <glow/common/hash.hh>

#include "renderables/Renderable.hh"

using namespace glow;
using namespace glow::viewer;

void Scene::add(const SharedRenderable& r) { mRenderables.push_back(r); }

aabb Scene::computeAabb() const
{
    aabb bounds;
    for (auto const& r : mRenderables)
        if (!r->isEmpty())
            bounds.add(r->computeAabb());
    return bounds;
}

size_t Scene::queryHash() const
{
    if (mCachedHash.has_value())
        return mCachedHash.value();

    std::vector<size_t> data;
    data.reserve(1 + mRenderables.size());
    data.push_back(config.computeHash());
    for (auto const& r : mRenderables)
        data.push_back(r->queryHash()); // was already computed on init
    mCachedHash = glow::hash_xxh3(as_byte_view(data), 0x912312);

    return mCachedHash.value();
}

bool Scene::has_3d_renderables() const { return !computeAabb().isEmpty(); }

bool Scene::shouldBeCleared() const
{
    if (config.clearAccumulation)
        return true;
    for (auto const& r : mRenderables)
        if (r->isDirty())
            return true;
    return false;
}

Scene::BoundingInfo Scene::getBoundingInfo() const
{
    auto aabb = computeAabb();
    if (aabb.isEmpty())
    {
        // Centered unit aabb for algorithm sanity down the line
        aabb.min = tg::pos3(-.5f);
        aabb.max = tg::pos3(.5f);
    }

    if (config.customBounds.has_value())
    {
        aabb.min = config.customBounds.value().min;
        aabb.max = config.customBounds.value().max;
    }

    auto const size = aabb.size();
    return BoundingInfo{length(size), aabb.center(), tg::aabb3(aabb.min, aabb.max)};
}
