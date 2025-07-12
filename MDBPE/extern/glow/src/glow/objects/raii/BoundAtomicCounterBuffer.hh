#pragma once

#include <initializer_list>
#include <vector>

#include <clean-core/span.hh>

#include <glow/common/non_copyable.hh>
#include <glow/fwd.hh>
#include <glow/gl.hh>

namespace glow
{
/// RAII-object that defines a "bind"-scope for an AtomicCounterBuffer
/// All functions that operate on the currently bound buffer are accessed here
struct BoundAtomicCounterBuffer
{
    GLOW_RAII_CLASS(BoundAtomicCounterBuffer);

    /// Backreference to the buffer
    AtomicCounterBuffer* const buffer;

    /// Sets all counters to the given values (discards all previous data)
    void setCounters(cc::span<uint32_t const> values, GLenum usage = GL_STATIC_DRAW);
    void setCounters(std::initializer_list<uint32_t> values, GLenum usage = GL_STATIC_DRAW)
    {
        setCounters(cc::span(values.begin(), values.size()), usage);
    }
    void setCounters(uint32_t value, int count = 1, GLenum usage = GL_STATIC_DRAW);

    /// Returns the value of a counter at the given position
    uint32_t getCounter(int pos = 0);
    /// Returns the value of all counters
    std::vector<uint32_t> getCounters();

    /// Returns the size in bytes of this buffer
    size_t getByteSize() const;
    /// Returns the number of counters in this buffer
    int getCounterSize() const;

private:
    GLint previousBuffer;                        ///< previously bound buffer
    BoundAtomicCounterBuffer* previousBufferPtr; ///< previously bound buffer
    BoundAtomicCounterBuffer(AtomicCounterBuffer* buffer);
    friend class AtomicCounterBuffer;

    /// returns true iff it's safe to use this bound class
    /// otherwise, runtime error
    bool isCurrent() const;

public:
    BoundAtomicCounterBuffer(BoundAtomicCounterBuffer&&); // allow move
    ~BoundAtomicCounterBuffer();
};
}
