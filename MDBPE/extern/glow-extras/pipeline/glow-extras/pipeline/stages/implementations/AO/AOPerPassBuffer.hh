#pragma once

#include <glow/std140.hh>

namespace glow
{
namespace pipeline
{
namespace detail
{
struct AOPerPassUBO
{
    std140vec4 jitter;
    std140vec2 offset;
    std140float fSliceIndex;
    std140uint uSliceIndex;
};

class AOPerPassBuffer
{
public:
    AOPerPassUBO data;

    AOPerPassBuffer();

    void setOffset(unsigned offX, unsigned offY);
    void setJitter(tg::vec4 const& jitter);
    void setSliceIndex(unsigned index);
};
}
}
}
