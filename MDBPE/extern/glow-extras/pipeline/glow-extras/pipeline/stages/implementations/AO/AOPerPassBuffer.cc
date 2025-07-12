#include "AOPerPassBuffer.hh"

#include <cstring>

glow::pipeline::detail::AOPerPassBuffer::AOPerPassBuffer()
{
    memset(&data, 0, sizeof(data));
}

void glow::pipeline::detail::AOPerPassBuffer::setOffset(unsigned offX, unsigned offY)
{
    data.offset = tg::vec2(static_cast<float>(offX) + .5f, static_cast<float>(offY) + .5f);
}

void glow::pipeline::detail::AOPerPassBuffer::setJitter(const tg::vec4& jitter)
{
    data.jitter = jitter;
}

void glow::pipeline::detail::AOPerPassBuffer::setSliceIndex(unsigned index)
{
    data.fSliceIndex = static_cast<float>(index);
    data.uSliceIndex = index;
}
