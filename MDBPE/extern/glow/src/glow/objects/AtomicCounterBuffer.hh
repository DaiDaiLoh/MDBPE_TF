#pragma once

#include <clean-core/span.hh>

#include <glow/common/log.hh>
#include <glow/common/nodiscard.hh>
#include <glow/common/shared.hh>
#include <glow/fwd.hh>

#include "Buffer.hh"

#include "raii/BoundAtomicCounterBuffer.hh"

#include <string>

namespace glow
{
GLOW_SHARED(class, AtomicCounterBuffer);
/**
 * A buffer holding atomic counters
 *
 * Counters are always uint32_t, one buffer may hold multiple counters
 *
 * Usage in C++:
 *   program->setAtomicCounterBuffer(bindingPos, buffer);
 *
 * Usage in shader: (offset in byte)
 *      layout(binding = 0) uniform atomic_uint myArbitraryName1;
 *      layout(binding = 2, offset = 4) uniform atomic_uint myArbitraryName2;
 */
class AtomicCounterBuffer final : public Buffer
{
public: // getter
    /// Gets the currently bound AtomicCounterBuffer (nullptr if none)
    static BoundAtomicCounterBuffer* getCurrentBuffer();

public:
    AtomicCounterBuffer();

    /// Binds this uniform buffer.
    /// Unbinding is done when the returned object runs out of scope.
    GLOW_NODISCARD BoundAtomicCounterBuffer bind() { return {this}; }

public: // static construction
    /// Creates an atomic counter buffer with given initial value and count
    /// NOTE: default is a single counter with value 0
    static SharedAtomicCounterBuffer create(uint32_t value = 0, int count = 1);
    /// Creates an atomic counter buffer with given initial values
    static SharedAtomicCounterBuffer create(cc::span<uint32_t const> values);
    static SharedAtomicCounterBuffer create(std::initializer_list<uint32_t> values) { return create(cc::span(values.begin(), values.size())); }
};
}
