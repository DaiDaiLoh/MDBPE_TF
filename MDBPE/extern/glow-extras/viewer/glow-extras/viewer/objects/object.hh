#pragma once

#include "../fwd.hh"

namespace glow
{
namespace viewer
{
/// these are temporary builder objects and never saved!
struct object
{
    object() = default;

    // no move
    object(object&&) = delete;
    // no copy, no assignments
    object(object const&) = delete;
    object& operator=(object const&) = delete;
    object& operator=(object&&) = delete;

    virtual ~object() {}
};
}
}
