#pragma once

#include <array>

#include <glow/common/non_copyable.hh>
#include <glow/fwd.hh>
#include <glow/gl.hh>

#include <typed-geometry/tg-lean.hh>

#include "../FramebufferAttachment.hh"

namespace glow
{
/// RAII-object that defines a "bind"-scope for a Framebuffer
/// All functions that operate on the currently bound buffer are accessed here
///
/// Notes:
/// - attachXyz will update viewports if AutoViewport is enabled
struct BoundFramebuffer
{
    GLOW_RAII_CLASS(BoundFramebuffer);

    /// Backreference to the buffer
    Framebuffer* const buffer;

    /// Check if the FBO is complete
    /// returns true iff complete
    bool checkComplete();

    /// Reattaches all FBO attachments
    void reattach();

    /// Attaches a texture to a named fragment location (color output)
    /// Overrides previously bound textures to the same name
    /// nullptr is a valid texture
    void attachColor(std::string const& fragName, SharedTexture const& tex, int mipmapLevel = 0, int layer = 0);
    void attachColor(FramebufferAttachment const& a);
    /// Attaches a texture to the depth target
    /// nullptr is a valid texture
    void attachDepth(SharedTexture const& tex, int mipmapLevel = 0, int layer = 0);
    /// Attaches a texture to the stencil target
    /// nullptr is a valid texture
    void attachStencil(SharedTexture const& tex, int mipmapLevel = 0, int layer = 0);
    /// Attaches a texture to the depth AND stencil target
    /// nullptr is a valid texture
    void attachDepthStencil(SharedTexture const& tex, int mipmapLevel = 0, int layer = 0);
    /// Creates a new depth texture with given format and attaches it
    /// Requires already attached color textures! (for size)
    /// May bind an texture
    void attachDepth(GLenum depthFormat = GL_DEPTH_COMPONENT32);

    /// Override the default, attachment-based viewport setting
    void setViewport(tg::isize2 const& size);

private:
    GLint previousBuffer;                      ///< previously bound buffer
    std::array<GLenum, 8> previousDrawBuffers; ///< previously setup draw buffers
    std::array<GLint, 4> previousViewport;     ///< previously configure viewport
    BoundFramebuffer* previousBufferPtr;       ///< previously bound buffer
    BoundFramebuffer(Framebuffer* buffer);
    friend class Framebuffer;

    /// returns true iff it's safe to use this bound class
    /// otherwise, runtime error
    bool isCurrent() const;

    void updateViewport();

public:
    BoundFramebuffer(BoundFramebuffer&&); // allow move
    ~BoundFramebuffer();
};
}
