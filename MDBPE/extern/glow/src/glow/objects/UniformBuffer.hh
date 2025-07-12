#pragma once

#include <map>
#include <string>
#include <string_view>

#include <glow/common/nodiscard.hh>
#include <glow/common/property.hh>
#include <glow/common/shared.hh>
#include <glow/common/string_map.hh>

#include "Buffer.hh"

#include "raii/BoundUniformBuffer.hh"


namespace glow
{
GLOW_SHARED(class, UniformBuffer);
/**
 * A uniform buffer for generic data
 *
 * Usage:
 *   program->setUniformBuffer("bufferName", buffer);
 *
 * See std140.hh for ways to safely use C++ structs
 *
 * addVerification(...) can be used to add offset-name pairs to be verified *
 * Usage:
 *   buffer->addVerification(offsetAsInt, "nameInBuffer");
 *   buffer->addVerification(&MyStruct::varName, "nameInBuffer");
 *   buffer->addVerification({{&MyStruct::varName, "nameInBuffer"}, ... });
 * Caution:
 *   * will only check when bound to particular shader
 *   * may not work if variable is not used in shader
 */
class UniformBuffer final : public Buffer
{
private:
    /// Offset to verify
    util::string_map<int> mVerificationOffsets;

public: // getter
    /// Gets the currently bound UniformBuffer (nullptr if none)
    static BoundUniformBuffer* getCurrentBuffer();

    GLOW_GETTER(VerificationOffsets);

public:
    UniformBuffer();

    /// Adds a verification target to shader
    /// Emits warnings/erorrs when expected offset does not match reported one (when used in shader)
    void addVerification(int offset, std::string_view nameInShader);
    template <typename StructT, typename DataT>
    void addVerification(DataT StructT::*member, std::string_view nameInShader)
    {
        addVerification(int(reinterpret_cast<std::ptrdiff_t>(&(reinterpret_cast<StructT const volatile*>(0)->*member))), nameInShader);
    }
    struct VerificationPair
    {
        int offset;
        std::string name;

        VerificationPair() = default;
        template <typename StructT, typename DataT>
        VerificationPair(DataT StructT::*member, std::string nameInShader)
          : offset(reinterpret_cast<std::ptrdiff_t>(&(reinterpret_cast<StructT const volatile*>(0)->*member))), name(std::move(nameInShader))
        {
        }
    };
    void addVerification(cc::span<VerificationPair const> members)
    {
        for (auto const& p : members)
            addVerification(p.offset, p.name);
    }
    void addVerification(std::initializer_list<VerificationPair> members)
    {
        for (auto const& p : members)
            addVerification(p.offset, p.name);
    }

    /// Binds this uniform buffer.
    /// Unbinding is done when the returned object runs out of scope.
    GLOW_NODISCARD BoundUniformBuffer bind() { return {this}; }

public: // static construction
    /// Creates an empty array buffer
    /// Same as std::make_shared<UniformBuffer>();
    static SharedUniformBuffer create();
};
}
