#pragma once

#include <glow/common/nodiscard.hh>
#include <glow/common/non_copyable.hh>
#include <glow/common/shared.hh>

#include <glow/gl.hh>

#include "NamedObject.hh"

#include "raii/BoundFramebuffer.hh"

#include <clean-core/span.hh>

#include <array>
#include <string>
#include <vector>

namespace glow
{
GLOW_SHARED(class, Framebuffer);
GLOW_SHARED(class, LocationMapping);
GLOW_SHARED(class, Texture);

/**
 * @brief The Framebuffer class
 *
 * For CubeMaps, layer = 0 (GL_TEXTURE_CUBE_MAP_POSITIVE_X) .. 5 (GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
 * For CubeMap Arrays, layer is (array index * 6 + cubemap layer)
 */
class Framebuffer final : public NamedObject<Framebuffer, GL_FRAMEBUFFER>
{
private:
    /// OGL id
    GLuint mObjectName;

    /// Location mapping from fragment output name to loc
    /// At any point, the mapping saved here must be consistent (i.e. a superset) of the GPU mapping
    SharedLocationMapping mFragmentMapping;
    friend class VertexArray;       // for negotiation
    friend struct BoundVertexArray; // for negotiation

    /// List of color a.
    std::vector<FramebufferAttachment> mColorAttachments;
    /// Current depth attachment
    FramebufferAttachment mDepthAttachment = {"", nullptr, -1, -1};
    /// Current stencil attachment
    FramebufferAttachment mStencilAttachment = {"", nullptr, -1, -1};

    /// if true, sets the viewport automatically to the minimal attached size
    bool mAutoViewport = true;

private:
    /// Careful! must be bound
    void internalReattach();
    /// Careful! must be bound
    bool internalCheckComplete();

public: // getter
    GLuint getObjectName() const { return mObjectName; }
    SharedLocationMapping const& getFragmentMapping() const { return mFragmentMapping; }
    std::vector<FramebufferAttachment> const& getColorAttachments() const { return mColorAttachments; }
    FramebufferAttachment const& getDepthAttachment() const { return mDepthAttachment; }
    FramebufferAttachment const& getStencilAttachment() const { return mStencilAttachment; }
    void setAutoViewport(bool enable) { mAutoViewport = enable; }
    bool getAutoViewport() const { return mAutoViewport; }
    /// Gets the currently bound FBO (nullptr if none)
    static BoundFramebuffer* getCurrentBuffer();

public:
    /// Notifies this FBO that some shader wrote to it
    /// E.g. used to tell textures that their mipmaps are invalid
    void notifyShaderExecuted();

public:
    Framebuffer();
    ~Framebuffer();

    /// Binds this framebuffer.
    /// Unbinding is done when the returned object runs out of scope.
    GLOW_NODISCARD BoundFramebuffer bind() { return {this}; }
    friend BoundFramebuffer;

public: // static construction
    /// Creates an empty framebuffer
    /// Same as std::make_shared<Framebuffer>();
    static SharedFramebuffer create();
    /// Creates a framebuffer with given color, depth, and stencil attachements
    /// The order of colors matches the target indices (e.g. for glClearBuffer or GL_DRAW_BUFFERi)
    /// CAUTION: This framebuffer is checked for completeness!
    static SharedFramebuffer create(cc::span<FramebufferAttachment const> colors,
                                    SharedTexture const& depth = nullptr,
                                    SharedTexture const& stencil = nullptr,
                                    int depthStencilMipmapLevel = 0,
                                    int depthStencilLayer = 0);
    static SharedFramebuffer create(std::initializer_list<FramebufferAttachment> colors,
                                    SharedTexture const& depth = nullptr,
                                    SharedTexture const& stencil = nullptr,
                                    int depthStencilMipmapLevel = 0,
                                    int depthStencilLayer = 0)
    {
        return create(cc::span(colors.begin(), colors.size()), depth, stencil, depthStencilMipmapLevel, depthStencilLayer);
    }
    /// Shortcut for create({{fragmentName, color}}, ...)
    static SharedFramebuffer create(std::string const& fragmentName,
                                    SharedTexture const& color,
                                    SharedTexture const& depth = nullptr,
                                    SharedTexture const& stencil = nullptr,
                                    int depthStencilMipmapLevel = 0,
                                    int depthStencilLayer = 0);
    /// Creates a framebuffer without attached colors
    /// CAUTION: This framebuffer is checked for completeness!
    static SharedFramebuffer createDepthOnly(SharedTexture const& depth, SharedTexture const& stencil = nullptr, int depthStencilMipmapLevel = 0, int depthStencilLayer = 0);
};
}
