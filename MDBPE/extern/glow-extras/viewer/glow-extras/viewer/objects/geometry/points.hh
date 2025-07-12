#pragma once

#include "PointBuilder.hh"

namespace glow
{
namespace viewer
{
template <class ObjectT>
auto points(ObjectT const& pos) -> decltype(builder::PointBuilder(detail::make_mesh_definition(pos)))
{
    return builder::PointBuilder(detail::make_mesh_definition(pos));
}
}
}
