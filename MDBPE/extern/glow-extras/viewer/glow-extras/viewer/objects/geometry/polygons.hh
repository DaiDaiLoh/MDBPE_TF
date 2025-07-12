#pragma once

#include "PolygonBuilder.hh"

namespace glow
{
namespace viewer
{
template <class PosAttributeT>
auto polygons(PosAttributeT const& pos) -> decltype(builder::PolygonBuilder(detail::make_mesh_definition(pos)))
{
    return builder::PolygonBuilder(detail::make_mesh_definition(pos));
}
}
}
