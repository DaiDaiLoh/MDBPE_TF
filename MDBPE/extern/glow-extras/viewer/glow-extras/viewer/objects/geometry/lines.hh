#pragma once

#include "LineBuilder.hh"

namespace glow
{
namespace viewer
{
template <class ObjectT>
auto lines(ObjectT const& pos) -> decltype(builder::LineBuilder(detail::make_mesh_definition(pos)))
{
    return builder::LineBuilder(detail::make_mesh_definition(pos));
}
}
}
