#pragma once

#include "Masking.hh"

namespace glow
{
namespace viewer
{
template <class T>
Masking masked(T const& data)
{
    return Masking().visibilityMask(data);
}
template <class T>
Masking masked(T const& data, float threshold)
{
    return masked(data).threshold(threshold);
}
}
}

namespace polymesh
{
// insert into polymesh namespace so ADL works
// e.g. view(pos, masked(data)) works without using any namespace (pos is polymesh::vertex_attribute<glm::vec3>)
using glow::viewer::masked;
}
