// This file is auto-generated and should not be modified directly.
#pragma once

#include "Texture.hh"

#include <glow/common/gltypeinfo.hh>
#include <glow/common/nodiscard.hh>
#include <glow/common/log.hh>

#include <glow/data/ColorSpace.hh>

#include <vector>

#include <clean-core/span.hh>

#include <typed-geometry/tg-lean.hh>

namespace glow
{
GLOW_SHARED(class, Texture2DMultisampleArray);
GLOW_SHARED(class, TextureData);

/// Defines an array of 2D multisampled textures in OpenGL
class Texture2DMultisampleArray final : public Texture
{
public:
    struct BoundTexture2DMultisampleArray;

    using SizeT = tg::isize3;

    struct Shape
    {
        GLenum format;
        SizeT size;
        int samples;

        inline bool operator==(Shape const& rhs) const
        {
            return (format == rhs.format) && (size == rhs.size) && (samples == rhs.samples);
        }
    };

private:

    /// Minification filter
    GLenum mMinFilter = GL_NEAREST_MIPMAP_LINEAR;
    /// Magnification filter
    GLenum mMagFilter = GL_LINEAR;

    /// Border color
    tg::color4 mBorderColor = {0.0f, 0.0f, 0.0f, 0.0f};

    /// Wrapping in S
    GLenum mWrapS = GL_REPEAT;
    /// Wrapping in T
    GLenum mWrapT = GL_REPEAT;

    /// Comparison mode
    GLenum mCompareMode = GL_NONE;
    /// Comparison function
    GLenum mCompareFunc = GL_LESS;

    /// Depth/Stencil read mode
    GLenum mDepthStencilMode = GL_DEPTH_COMPONENT;

    /// Level of anisotropic filtering (>= 1.f, which is isotropic)
    /// Max number of samples basically
    GLfloat mAnisotropicFiltering = 1.0f;

    /// Texture size: Width
    int mWidth = 0u;
    /// Texture size: Height
    int mHeight = 0u;
    /// Texture size: Layers
    int mLayers = 0u;


    /// Level of multisampling
    int mSampleAmount = 4;
    /// Whether to use identical sample locations and -amounts for all texels
    GLboolean mFixedSamples = GL_FALSE;


public: // getter
    /// Gets the currently bound texture (nullptr if none)
    static BoundTexture2DMultisampleArray* getCurrentTexture();

    GLenum getMinFilter() const { return mMinFilter; }
    GLenum getMagFilter() const { return mMagFilter; }
    tg::color4 getBorderColor() const { return mBorderColor; }
    GLenum getWrapS() const { return mWrapS; }
    GLenum getWrapT() const { return mWrapT; }
    GLenum getCompareMode() const { return mCompareMode; }
    GLenum getCompareFunc() const { return mCompareFunc; }
    GLenum getDepthStencilMode() const { return mDepthStencilMode; }
    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }
    int getLayers() const { return mLayers; }
    tg::isize3 getDimensions() const override { return { mWidth, mHeight, mLayers }; }
    SizeT getSize() const { return { mWidth, mHeight, mLayers }; }
    
    Shape getShape() const { return { mInternalFormat, getSize(), mSampleAmount }; }



    int getSampleAmount() const { return mSampleAmount; }
    GLboolean hasFixedSamples() const { return mFixedSamples; }

    /// returns the uniform type that should be used for sampling this texture
    GLenum getUniformType() const override;

public:
    /// RAII-object that defines a "bind"-scope for an array of 2D multisampled textures
    /// All functions that operate on the currently bound tex are accessed here
    struct BoundTexture2DMultisampleArray
    {
        GLOW_RAII_CLASS(BoundTexture2DMultisampleArray);

        /// Backreference to the texture
        Texture2DMultisampleArray* const texture;


        /// Sets minification filter (GL_NEAREST, GL_LINEAR)
        void setMinFilter(GLenum filter);
        /// Sets magnification filter (GL_NEAREST, GL_LINEAR)
        void setMagFilter(GLenum filter);
        /// Sets mag and min filter
        void setFilter(GLenum magFilter, GLenum minFilter);

        /// Sets the number of anisotropic samples (>= 1)
        void setAnisotropicFiltering(GLfloat samples);

        /// Sets the border color
        void setBorderColor(tg::color4 const& color);

        /// Sets texture wrapping in S
        void setWrapS(GLenum wrap);
        /// Sets texture wrapping in T
        void setWrapT(GLenum wrap);
        /// Sets texture wrapping in all directions
        void setWrap(GLenum wrapS, GLenum wrapT);

        /// Sets the texture compare mode (must be enabled for shadow samplers)
        /// Valid values: GL_COMPARE_REF_TO_TEXTURE and GL_NONE
        void setCompareMode(GLenum mode);
        /// Sets the function for comparison (LESS, LEQUAL, ...)
        void setCompareFunc(GLenum func);
        /// Sets the depth/stencil texture mode (GL_DEPTH_COMPONENT or GL_STENCIL_COMPONENT)
        void setDepthStencilMode(GLenum mode);


        /// Resizes the texture
        /// invalidates the data
        void resize(int width, int height, int layers, int samples = 4, GLboolean fixedSamples = GL_FALSE);
        void resize(SizeT size, int samples = 4, GLboolean fixedSamples = GL_FALSE) { resize(size.width, size.height, size.depth, samples, fixedSamples); }

        /// Sets the multisampling properties
        /// Invalidates the data
        void setMultisampling(int samples, GLboolean fixedSamples);
        void setSampleAmount(int samples) { setMultisampling(samples, texture->mFixedSamples); }
        void setFixedSamples(GLboolean fixedSamples) { setMultisampling(texture->mSampleAmount, fixedSamples); }


    private:
        GLint previousTexture;              ///< previously bound tex
        BoundTexture2DMultisampleArray* previousTexturePtr; ///< previously bound tex
        BoundTexture2DMultisampleArray (Texture2DMultisampleArray* buffer);
        friend class Texture2DMultisampleArray;

        /// returns true iff it's safe to use this bound class
        /// otherwise, runtime error
        bool isCurrent() const;

    public:
        BoundTexture2DMultisampleArray (BoundTexture2DMultisampleArray &&); // allow move
        ~BoundTexture2DMultisampleArray ();
    };
public:

    /// Fills the specific mipmap level (default 0) with the given data
    /// Requires OpenGL 4.4 (for now) and will throw a run-time error otherwise
    void clear(GLenum format, GLenum type, const GLvoid* data);
    /// Clear via glm, tg, or c++ type (see gltypeinfo)
    /// CAREFUL: pointers do not work!
    template <typename DataT>
    void clear(DataT const& data)
    {
        clear(glTypeOf<DataT>::format, glTypeOf<DataT>::type, (const GLvoid*)&data);
    }

public:
    Texture2DMultisampleArray(GLenum internalFormat = GL_RGBA);
    Texture2DMultisampleArray(Shape const& shape);

    /// Binds this texture.
    /// Unbinding is done when the returned object runs out of scope.
    GLOW_NODISCARD BoundTexture2DMultisampleArray bind() { return {this}; }
public: // static construction
    /// Creates an array of 2D multisampled textures with given width and height
    static SharedTexture2DMultisampleArray create(int width = 1, int height = 1, int layers = 1, GLenum internalFormat = GL_RGBA);
    /// Creates an array of 2D multisampled textures from Shape
    static SharedTexture2DMultisampleArray create(Shape const& shape);
    static SharedTexture2DMultisampleArray create(SizeT size, GLenum internalFormat = GL_RGBA) { return create(size.width, size.height, size.depth, internalFormat); }


    friend class Framebuffer;
};
}
