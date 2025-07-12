#include "Framebuffer.hh"

#include <typed-geometry/feature/assert.hh>
#include <typed-geometry/feature/vector.hh>

#include <limits>

#include "Program.hh"
#include "Texture.hh"
#include "TextureRectangle.hh"

#include <glow/glow.hh>

#include <glow/common/ogl_typeinfo.hh>
#include <glow/common/runtime_assert.hh>
#include <glow/common/thread_local.hh>

#include <glow/util/LocationMapping.hh>

using namespace glow;

/// Currently bound buffer
static GLOW_THREADLOCAL BoundFramebuffer* sCurrentFramebuffer = nullptr;

static void attachToFramebuffer(SharedTexture const& tex, GLenum attachment, int mipmapLevel, int layer)
{
    checkValidGLOW();

    if (!tex)
    {
        glFramebufferTexture(GL_FRAMEBUFFER, attachment, 0, 0);
        return;
    }

    // layered attachment, type-independent
    if (layer == -1)
    {
        glFramebufferTexture(GL_FRAMEBUFFER, attachment, tex->getObjectName(), mipmapLevel);
        return;
    }

    switch (tex->getTarget())
    {
    case GL_TEXTURE_1D:
        glFramebufferTexture1D(GL_FRAMEBUFFER, attachment, tex->getTarget(), tex->getObjectName(), mipmapLevel);
        break;
    case GL_TEXTURE_2D:
    case GL_TEXTURE_RECTANGLE:
    case GL_TEXTURE_2D_MULTISAMPLE:
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, tex->getTarget(), tex->getObjectName(), mipmapLevel);
        break;
    case GL_TEXTURE_CUBE_MAP:
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, tex->getObjectName(), mipmapLevel);
        break;
    case GL_TEXTURE_3D:
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
        glFramebufferTexture3D(GL_FRAMEBUFFER, attachment, tex->getTarget(), tex->getObjectName(), mipmapLevel, layer);
        break;
    case GL_TEXTURE_1D_ARRAY:
    case GL_TEXTURE_2D_ARRAY:
    case GL_TEXTURE_CUBE_MAP_ARRAY:
        glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, tex->getObjectName(), mipmapLevel, layer);
        break;
    case GL_TEXTURE_BUFFER:
    default:
        error() << "Unsupported texture target " << tex->getTarget() << " " << to_string(tex.get());
        break;
    }
}

void Framebuffer::internalReattach()
{
    TG_ASSERT(sCurrentFramebuffer && sCurrentFramebuffer->buffer == this);
    checkValidGLOW();

    // clear attachments
    GLint maxAttach = 0;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);
    for (auto i = 0; i < maxAttach; ++i)
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, 0, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);

    // reattach
    for (auto const& a : mColorAttachments)
    {
        auto loc = mFragmentMapping->getOrAddLocation(a.locationName);
        attachToFramebuffer(a.texture, GL_COLOR_ATTACHMENT0 + loc, a.mipmapLevel, a.layer);
    }
    if (mDepthAttachment.texture)
        attachToFramebuffer(mDepthAttachment.texture, GL_DEPTH_ATTACHMENT, mDepthAttachment.mipmapLevel, mDepthAttachment.layer);
    if (mStencilAttachment.texture)
        attachToFramebuffer(mStencilAttachment.texture, GL_STENCIL_ATTACHMENT, mStencilAttachment.mipmapLevel, mStencilAttachment.layer);
}

bool Framebuffer::internalCheckComplete()
{
    TG_ASSERT(sCurrentFramebuffer && sCurrentFramebuffer->buffer == this);
    checkValidGLOW();

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (status == GL_FRAMEBUFFER_COMPLETE)
        return true;

    if (mDepthAttachment.texture && mStencilAttachment.texture && mDepthAttachment.texture->getObjectName() != mStencilAttachment.texture->getObjectName())
        error() << "NOTE: depth and stencil buffer are not the same texture. Drivers are not required to support this.";

    switch (status)
    {
    case GL_FRAMEBUFFER_UNDEFINED:
        error() << "Incomplete Framebuffer " << to_string(this);
        error() << "Framebuffer not complete: GL_FRAMEBUFFER_UNDEFINED is returned if the specified framebuffer is the "
                   "default read or draw framebuffer, but the default framebuffer does not exist.";
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        error() << "Incomplete Framebuffer " << to_string(this);
        error() << "Framebuffer not complete: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT is returned if any of the "
                   "framebuffer attachment points are framebuffer incomplete.";
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        error() << "Incomplete Framebuffer " << to_string(this);
        error() << "Framebuffer not complete: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT is returned if the "
                   "framebuffer does not have at least one image attached to it.";
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        error() << "Incomplete Framebuffer " << to_string(this);
        error() << "Framebuffer not complete: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER is returned if the value of "
                   "GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by "
                   "GL_DRAW_BUFFERi.";
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        error() << "Incomplete Framebuffer " << to_string(this);
        error() << "Framebuffer not complete: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER is returned if GL_READ_BUFFER is "
                   "not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color "
                   "attachment point named by GL_READ_BUFFER.";
        return false;

    case GL_FRAMEBUFFER_UNSUPPORTED:
        error() << "Incomplete Framebuffer " << to_string(this);
        error() << "Framebuffer not complete: GL_FRAMEBUFFER_UNSUPPORTED is returned if the combination of internal "
                   "formats of the attached images violates an implementation-dependent set of restrictions.";
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        error() << "Incomplete Framebuffer " << to_string(this);
        error() << "Framebuffer not complete: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is returned if the value of "
                   "GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers; if the value of "
                   "GL_TEXTURE_SAMPLES is the not same for all attached textures; or, if the attached images are a mix "
                   "of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of "
                   "GL_TEXTURE_SAMPLES.\n"
                   "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is also returned if the value of "
                   "GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or, if the attached "
                   "images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is "
                   "not GL_TRUE for all attached textures.";
        return false;

    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        error() << "Incomplete Framebuffer " << to_string(this);
        error() << "Framebuffer not complete: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS is returned if any framebuffer "
                   "attachment is layered, and any populated attachment is not layered, or if all populated color "
                   "attachments are not from textures of the same target.";
        return false;

    default:
        error() << "Incomplete Framebuffer " << to_string(this);
        error() << "Unknown framebuffer status: " << status;
        break;
    }

    return false;
}

BoundFramebuffer* Framebuffer::getCurrentBuffer() { return sCurrentFramebuffer; }

void Framebuffer::notifyShaderExecuted()
{
    // invalidate texture mipmaps
    for (auto const& a : mColorAttachments)
        if (a.texture != nullptr)
            a.texture->setMipmapsGenerated(false);

    if (mDepthAttachment.texture != nullptr)
        mDepthAttachment.texture->setMipmapsGenerated(false);

    if (mStencilAttachment.texture != nullptr)
        mStencilAttachment.texture->setMipmapsGenerated(false);
}

Framebuffer::Framebuffer()
{
    checkValidGLOW();

    mObjectName = std::numeric_limits<decltype(mObjectName)>::max();
    glGenFramebuffers(1, &mObjectName);
    TG_ASSERT(mObjectName != std::numeric_limits<decltype(mObjectName)>::max() && "No OpenGL Context?");

    // bind the FB once to guarantee that object is valid
    GLint prevFB = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFB);
    glBindFramebuffer(GL_FRAMEBUFFER, mObjectName);
    // FB is now valid
    glBindFramebuffer(GL_FRAMEBUFFER, prevFB);

    mFragmentMapping = std::make_shared<LocationMapping>();
}

Framebuffer::~Framebuffer()
{
    checkValidGLOW();
    glDeleteFramebuffers(1, &mObjectName);
}

SharedFramebuffer Framebuffer::create() { return std::make_shared<Framebuffer>(); }

SharedFramebuffer Framebuffer::create(cc::span<FramebufferAttachment const> colors, const SharedTexture& depth, const SharedTexture& stencil, int depthStencilMipmapLevel, int depthStencilLayer)
{
    auto fbo = create();
    auto bfbo = fbo->bind();
    for (auto const& a : colors)
        bfbo.attachColor(a);
    if (depth)
        bfbo.attachDepth(depth, depthStencilMipmapLevel, depthStencilLayer);
    if (stencil)
        bfbo.attachStencil(stencil, depthStencilMipmapLevel, depthStencilLayer);
    if (!bfbo.checkComplete())
        return nullptr;
    return fbo;
}

SharedFramebuffer Framebuffer::create(
    const std::string& fragmentName, const SharedTexture& color, const SharedTexture& depth, const SharedTexture& stencil, int depthStencilMipmapLevel, int depthStencilLayer)
{
    return create({{fragmentName, color}}, depth, stencil, depthStencilMipmapLevel, depthStencilLayer);
}

SharedFramebuffer Framebuffer::createDepthOnly(const SharedTexture& depth, const SharedTexture& stencil, int depthStencilMipmapLevel, int depthStencilLayer)
{
    return create(std::vector<FramebufferAttachment>{}, depth, stencil, depthStencilMipmapLevel, depthStencilLayer);
}

bool BoundFramebuffer::isCurrent() const
{
    GLOW_RUNTIME_ASSERT(sCurrentFramebuffer == this, "Currently bound FBO does NOT match represented buffer " << to_string(buffer), return false);
    return true;
}

void BoundFramebuffer::updateViewport()
{
    auto sentinel = tg::max<int>();
    auto dim = tg::isize3(sentinel);
    if (buffer->mDepthAttachment.texture)
        dim = tg::min(dim, buffer->mDepthAttachment.texture->getDimensions());
    if (buffer->mStencilAttachment.texture)
        dim = tg::min(dim, buffer->mStencilAttachment.texture->getDimensions());
    for (auto const& a : buffer->mColorAttachments)
        if (a.texture)
            dim = tg::min(dim, a.texture->getDimensions());
    if (dim != tg::isize3(sentinel)) // at least one texture
        glViewport(0, 0, dim.width, dim.height);
}

bool BoundFramebuffer::checkComplete()
{
    if (!isCurrent())
        return false;

    return buffer->internalCheckComplete();
}

void BoundFramebuffer::reattach()
{
    if (!isCurrent())
        return;

    buffer->internalReattach();
}

void BoundFramebuffer::attachColor(const std::string& fragName, const SharedTexture& tex, int mipmapLevel, int layer)
{
    if (!isCurrent())
        return;

    auto ifmt = tex->getInternalFormat();
    if (isDepthInternalFormat(ifmt) || isStencilInternalFormat(ifmt))
    {
        error() << "Trying to attach a depth/stencil texture as color target to a framebuffer. " << to_string(buffer) << ", " << to_string(tex.get());
        return;
    }

    auto loc = buffer->mFragmentMapping->getOrAddLocation(fragName);

    attachToFramebuffer(tex, GL_COLOR_ATTACHMENT0 + loc, mipmapLevel, layer);
    for (auto& a : buffer->mColorAttachments)
        if (a.locationName == fragName)
        {
            a.texture = tex;
            a.mipmapLevel = mipmapLevel;
            a.layer = layer;
            updateViewport();
            return; // attachment name found
        }

    // not found: add it
    buffer->mColorAttachments.push_back({fragName, tex, mipmapLevel, layer});
    updateViewport();
}

void BoundFramebuffer::attachColor(const FramebufferAttachment& a) { attachColor(a.locationName, a.texture, a.mipmapLevel, a.layer); }

void BoundFramebuffer::attachDepth(const SharedTexture& tex, int mipmapLevel, int layer)
{
    if (!isCurrent())
        return;

    attachToFramebuffer(tex, GL_DEPTH_ATTACHMENT, mipmapLevel, layer);
    buffer->mDepthAttachment = {"", tex, mipmapLevel, layer};
    updateViewport();
}

void BoundFramebuffer::attachStencil(const SharedTexture& tex, int mipmapLevel, int layer)
{
    if (!isCurrent())
        return;

    attachToFramebuffer(tex, GL_STENCIL_ATTACHMENT, mipmapLevel, layer);
    buffer->mStencilAttachment = {"", tex, mipmapLevel, layer};
    updateViewport();
}

void BoundFramebuffer::attachDepthStencil(const SharedTexture& tex, int mipmapLevel, int layer)
{
    if (!isCurrent())
        return;

    attachToFramebuffer(tex, GL_DEPTH_STENCIL_ATTACHMENT, mipmapLevel, layer);
    buffer->mDepthAttachment = {"", tex, mipmapLevel, layer};
    buffer->mStencilAttachment = {"", tex, mipmapLevel, layer};
    updateViewport();
}

void BoundFramebuffer::attachDepth(GLenum depthFormat)
{
    if (!isCurrent())
        return;

    GLOW_RUNTIME_ASSERT(buffer->mColorAttachments.size() > 0, "Requires already attached color textures! " << to_string(buffer), return);
    checkValidGLOW();

    auto tex = buffer->mColorAttachments[0].texture;

    // save old
    GLint oldTex;
    glGetIntegerv(tex->getBindingTarget(), &oldTex);

    // bind color
    glBindTexture(tex->getTarget(), tex->getObjectName());

    // get dimensions
    GLint w, h;
    glGetTexLevelParameteriv(tex->getTarget(), buffer->mColorAttachments[0].mipmapLevel, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(tex->getTarget(), buffer->mColorAttachments[0].mipmapLevel, GL_TEXTURE_HEIGHT, &h);

    // restore
    glBindTexture(tex->getTarget(), static_cast<GLuint>(oldTex));

    // create and attach 2DRect depth texture
    attachDepth(TextureRectangle::create(w, h, depthFormat));

    // finally check for completeness again
    checkComplete();

    // no update viewport needed because size cannot change
}

void BoundFramebuffer::setViewport(const tg::isize2& size) { glViewport(0, 0, size.width, size.height); }

BoundFramebuffer::BoundFramebuffer(Framebuffer* buffer) : buffer(buffer)
{
    checkValidGLOW();

    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousBuffer);

    GLint readFBO = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);
    if (readFBO != previousBuffer)
        error() << "GLOW does not support differing READ and WRITE Framebuffer. " << to_string(buffer);

    for (auto i = 0u; i < previousDrawBuffers.size(); ++i)
        glGetIntegerv(GL_DRAW_BUFFER0 + i, reinterpret_cast<GLint*>(&previousDrawBuffers[i]));

    glBindFramebuffer(GL_FRAMEBUFFER, buffer->getObjectName());

    // optimized: a single buffer
    switch (buffer->mColorAttachments.size())
    {
    case 0:
        glDrawBuffer(GL_NONE);
        break;
    case 1:
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        break;
    default:
        GLenum drawBuffers[8] = {};
        for (auto i = 0u; i < buffer->mColorAttachments.size(); ++i)
            drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
        glDrawBuffers(static_cast<GLsizei>(buffer->mColorAttachments.size()), drawBuffers);
        break;
    }

    if (buffer->mAutoViewport)
    {
        glGetIntegerv(GL_VIEWPORT, previousViewport.data());
        updateViewport();
    }

    previousBufferPtr = sCurrentFramebuffer;
    sCurrentFramebuffer = this;
}

BoundFramebuffer::BoundFramebuffer(BoundFramebuffer&& rhs)
  : buffer(rhs.buffer), previousBuffer(rhs.previousBuffer), previousDrawBuffers(rhs.previousDrawBuffers), previousBufferPtr(rhs.previousBufferPtr)
{
    // invalidate rhs
    rhs.previousBuffer = -1;
    sCurrentFramebuffer = this;
}

template <size_t N>
static bool isSingleBuffer(std::array<GLenum, N> const& bufs)
{
    for (size_t i = 1; i < N; ++i)
        if (bufs[i] != GL_NONE)
            return false;

    return true;
}

BoundFramebuffer::~BoundFramebuffer()
{
    if (previousBuffer != -1) // if valid
    {
        checkValidGLOW();

        glBindFramebuffer(GL_FRAMEBUFFER, previousBuffer);
        sCurrentFramebuffer = previousBufferPtr;

        // special behavior: GL_BACK can only appear in glDrawBuffer
        // in general: optimized version for 0 or 1 buffers
        if (isSingleBuffer(previousDrawBuffers))
            glDrawBuffer(previousDrawBuffers[0]);
        else
            glDrawBuffers((GLsizei)previousDrawBuffers.size(), previousDrawBuffers.data());

        if (buffer->mAutoViewport)
            glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    }
}
