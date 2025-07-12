#pragma once

#include "Buffer.hh"

#include <glow/common/nodiscard.hh>

#include <vector>

#include "raii/BoundElementArrayBuffer.hh"

namespace glow
{
GLOW_SHARED(class, ElementArrayBuffer);

class ElementArrayBuffer final : public Buffer
{
private:
    /// Type of the index data.
    /// Supported:
    ///   * GL_UNSIGNED_BYTE
    ///   * GL_UNSIGNED_SHORT
    ///   * GL_UNSIGNED_INT
    GLenum mIndexType = GL_INVALID_ENUM;

    /// Number of indices
    int mIndexCount = 0;

public: // getter
    GLenum getIndexType() const { return mIndexType; }
    int getIndexCount() const { return mIndexCount; }
    /// Gets the currently bound EAB (nullptr if none)
    static BoundElementArrayBuffer* getCurrentBuffer();

public:
    ElementArrayBuffer();

    /// Binds this vertex array.
    /// Unbinding is done when the returned object runs out of scope.
    /// CAUTION: Cannot be used while a VAO is bound! (runtime error)
    GLOW_NODISCARD BoundElementArrayBuffer bind() { return {this}; }
    friend BoundElementArrayBuffer;

public: // static construction
    /// Creates an empty element array buffer
    /// Same as std::make_shared<ElementArrayBuffer>();
    static SharedElementArrayBuffer create();

    /// Creates a element array buffer with the given indices
    /// Automatically picks the correct index type
    /// CAUTION: Cannot be used while a VAO is bound! (runtime error)
    static SharedElementArrayBuffer create(cc::span<int16_t const> indices) { return create(int(indices.size()), indices.data()); }
    static SharedElementArrayBuffer create(cc::span<uint16_t const> indices) { return create(int(indices.size()), indices.data()); }
    static SharedElementArrayBuffer create(cc::span<int32_t const> indices) { return create(int(indices.size()), indices.data()); }
    static SharedElementArrayBuffer create(cc::span<uint32_t const> indices) { return create(int(indices.size()), indices.data()); }
    static SharedElementArrayBuffer create(int indexCount, const int16_t* data);
    static SharedElementArrayBuffer create(int indexCount, const uint16_t* data);
    static SharedElementArrayBuffer create(int indexCount, const int32_t* data);
    static SharedElementArrayBuffer create(int indexCount, const uint32_t* data);
    [[deprecated("8bit indices are slow on modern hardware")]] static SharedElementArrayBuffer create(cc::span<int8_t> const& indices)
    {
        return create(int(indices.size()), indices.data());
    }
    [[deprecated("8bit indices are slow on modern hardware")]] static SharedElementArrayBuffer create(cc::span<uint8_t> const& indices)
    {
        return create(int(indices.size()), indices.data());
    }
    [[deprecated("8bit indices are slow on modern hardware")]] static SharedElementArrayBuffer create(int indexCount, const int8_t* data);
    [[deprecated("8bit indices are slow on modern hardware")]] static SharedElementArrayBuffer create(int indexCount, const uint8_t* data);

    /// Creates a element array buffer with the given indices
    /// Type must be specified explicitly
    /// CAUTION: Cannot be used while a VAO is bound! (runtime error)
    static SharedElementArrayBuffer create(int indexCount, GLenum indexType, const void* data = nullptr);
};
}
