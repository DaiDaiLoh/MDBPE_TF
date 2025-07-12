#pragma once

#include <glow/common/log.hh>
#include <glow/common/nodiscard.hh>
#include <glow/common/shared.hh>

#include "Buffer.hh"

#include "raii/BoundShaderStorageBuffer.hh"

#include <map>
#include <string>

namespace glow
{
GLOW_SHARED(class, ShaderStorageBuffer);
/**
 * A shader buffer for generic data
 *
 * Usage:
 *   program->setShaderStorageBuffer("bufferName", buffer);
 *
 * See std140.hh for ways to safely use C++ structs
 *
 */
class ShaderStorageBuffer final : public Buffer
{
public: // getter
    /// Gets the currently bound ShaderStorageBuffer (nullptr if none)
    static BoundShaderStorageBuffer* getCurrentBuffer();

public:
    ShaderStorageBuffer(SharedBuffer const& originalBuffer = nullptr);

    /// Binds this uniform buffer.
    /// Unbinding is done when the returned object runs out of scope.
    GLOW_NODISCARD BoundShaderStorageBuffer bind() { return {this}; }

public: // static construction
    /// Creates an empty shader storage buffer
    /// Same as std::make_shared<ShaderStorageBuffer>();
    static SharedShaderStorageBuffer create();
    /// Data is uninitialized
    static SharedShaderStorageBuffer create(size_t sizeInBytes);
    template <class DataRangeT>
    static SharedShaderStorageBuffer create(DataRangeT&& data)
    {
        auto ssbo = create();
        ssbo->bind().setData(data);
        return ssbo;
    }

    /// Creates a shader storage buffer that shares memory with another buffer
    static SharedShaderStorageBuffer createAliased(SharedBuffer const& originalBuffer);
};
}
