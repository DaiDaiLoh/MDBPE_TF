#include "transform.hh"

#include <iostream>

#include <typed-geometry/tg-std.hh>

const glow::transform glow::transform::Identity = transform();

std::string glow::to_string(glow::transform const& t)
{
    std::stringstream ss;
    ss << "[" << t.position << ", " << t.rotation << ", " << t.scale << "]";
    return ss.str();
}

std::ostream& glow::operator<<(std::ostream& stream, glow::transform const& t)
{
    stream << to_string(t);
    return stream;
}
