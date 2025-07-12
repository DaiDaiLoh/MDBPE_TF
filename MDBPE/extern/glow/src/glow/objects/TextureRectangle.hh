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
GLOW_SHARED(class, TextureRectangle);
GLOW_SHARED(class, TextureData);

/// Defines a rectangular texture in OpenGL
class TextureRectangle final : public Texture
{
public:
    struct BoundTextureRectangle;

    using SizeT = tg::isize2;

    struct Shape
    {
        GLenum format;
        SizeT size;

        inline bool operator==(Shape const& rhs) const
        {
            return (format == rhs.format) && (size == rhs.size);
        }
    };

private:

    /// Minification filter
    GLenum mMinFilter = GL_LINEAR;
    /// Magnification filter
    GLenum mMagFilter = GL_LINEAR;

    /// Border color
    tg::color4 mBorderColor = {0.0f, 0.0f, 0.0f, 0.0f};

    /// Wrapping in S
    GLenum mWrapS = GL_CLAMP_TO_EDGE;
    /// Wrapping in T
    GLenum mWrapT = GL_CLAMP_TO_EDGE;

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



    /// if true, this texture got immutable storage by glTexStorage2D
    bool mStorageImmutable = false;

public: // getter
    /// Gets the currently bound texture (nullptr if none)
    static BoundTextureRectangle* getCurrentTexture();

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
    tg::isize3 getDimensions() const override { return { mWidth, mHeight, 1 }; }
    SizeT getSize() const { return { mWidth, mHeight }; }
    
    Shape getShape() const { return { mInternalFormat, getSize() }; }

    bool isStorageImmutable() const override { return mStorageImmutable; }



    /// returns the uniform type that should be used for sampling this texture
    GLenum getUniformType() const override;

public:
    /// RAII-object that defines a "bind"-scope for a rectangular texture
    /// All functions that operate on the currently bound tex are accessed here
    struct BoundTextureRectangle
    {
        GLOW_RAII_CLASS(BoundTextureRectangle);

        /// Backreference to the texture
        TextureRectangle* const texture;

        /// Makes the storage of this texture immutable
        /// It is an error to call this more than once
        /// It is an error to upload data with a different internal format at a later point
        /// It is an error to resize after storage was made immutable (unless it's the same size)
        /// Invalidates previously uploaded data
        void makeStorageImmutable(int width, int height, GLenum internalFormat);

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
        void resize(int width, int height);
        void resize(SizeT size) { resize(size.width, size.height); }


        /// Generic data uploads
        /// Changes internal format, width, height, and data
        void setData(GLenum internalFormat, int width, int height, GLenum format, GLenum type, const GLvoid* data);
        /// Data upload via glm, tg, or c++ type (see gltypeinfo)
        template <typename DataRangeT>
        void setData(GLenum internalFormat, int width, int height, DataRangeT&& data)
        {
            static_assert(cc::is_any_contiguous_range<DataRangeT>, "data must be a contiguous range!");
            auto data_span = cc::span(data);
            using DataT = std::decay_t<decltype(data_span[0])>;
            static_assert(std::is_trivially_copyable_v<DataT>, "data must be trivially copyable");

            if (int(data_span.size()) != width * height)
            {
                error() << "Texture size is " << width << " x " << height << " = " << width * height << " but " << data_span.size()
                        << " pixels are provided. " << to_string(texture);
                return;
            }
            setData(internalFormat, width, height, glTypeOf<DataT>::format, glTypeOf<DataT>::type, data_span.data());
        }
        /// same as above for initializer lists
        template <class DataT>
        void setData(GLenum internalFormat, int width, int height, std::initializer_list<DataT> data)
        {
            setData(internalFormat, width, height, cc::span<std::add_const_t<DataT>>(data.begin(), data.size()));
        }
        /// Same as above
        /// Usage:
        ///   tg::vec3 texData[][] = { ... }
        ///   // it's [height][width]
        ///   setData(iFormat, texData);
        template <typename DataT, int width, int height>
        void setData(GLenum internalFormat, const DataT(&data)[height][width] )
        {
            setData(internalFormat, width, height, glTypeOf<DataT>::format, glTypeOf<DataT>::type, data);
        }

        /// Generic partial data uploads
        /// Only changes data. Offset and size must be inside original bounds.
        void setSubData(int x, int y, int width, int height, GLenum format, GLenum type, const GLvoid* data);
        /// Partial data upload via glm, tg, or c++ type (see gltypeinfo)
        template <typename DataRangeT>
        void setSubData(int x, int y, int width, int height, DataRangeT&& data)
        {
            static_assert(cc::is_any_contiguous_range<DataRangeT>, "data must be a contiguous range!");
            auto data_span = cc::span(data);
            using DataT = std::decay_t<decltype(data_span[0])>;
            static_assert(std::is_trivially_copyable_v<DataT>, "data must be trivially copyable");

            if (int(data_span.size()) != width * height)
            {
                error() << "Texture size is " << width << " x " << height << " = " << width * height << " but " << data_span.size()
                        << " pixels are provided. " << to_string(texture);
                return;
            }
            setSubData(x, y, width, height, glTypeOf<DataT>::format, glTypeOf<DataT>::type, data_span.data());
        }
        /// Same as above
        /// Usage:
        ///   tg::vec3 texData[][] = { ... }
        ///   // it's [height][width]
        ///   setSubData(x, y, texData);
        template <typename DataT, int width, int height>
        void setSubData(int x, int y, const DataT(&data)[height][width] )
        {
            setSubData(x, y, width, height, glTypeOf<DataT>::format, glTypeOf<DataT>::type, data);
        }

        /// Sets texture data from surface data
        /// May set multiple levels at once
        /// May modify texture parameter
        void setData(GLenum internalFormat, SharedTextureData const& data);

        /// Generic data download
        std::vector<char> getData(GLenum format, GLenum type);
        /// Generic data download
        void getData(GLenum format, GLenum type, size_t bufferSize, void* buffer);
        /// Data download via glm, tg, or c++ type (see gltypeinfo)
        template <typename DataT>
        std::vector<DataT> getData()
        {
            std::vector<DataT> data;
            data.resize(texture->mWidth * texture->mHeight);
            getData(glTypeOf<DataT>::format, glTypeOf<DataT>::type, data.size() * sizeof(DataT), data.data());
            return std::move(data);
        }

        /* TODO: OpenGL <4.5 does not support subimage retrieval (in 4.5, https://www.opengl.org/sdk/docs/man/html/glGetTextureSubImage.xhtml can be used)
        /// Generic partial data download
        std::vector<char> getSubData(GLenum format, GLenum type, int x, int y, int width, int height);
        /// Generic partial data download
        void getSubData(GLenum format, GLenum type, int x, int y, int width, int height, size_t bufferSize, void* buffer);
        /// Partial data download via glm, tg, or c++ type (see gltypeinfo)
        template <typename DataT>
        std::vector<DataT> getSubData(int x, int y, int width, int height, int mipmapLevel = 0)
        {
            std::vector<DataT> data;
            data.resize(width * height);
            getSubData(glTypeOf<DataT>::format, glTypeOf<DataT>::type, x, y, width, height, data.size() * sizeof(DataT), data.data());
            return std::move(data);
        }
        */

        /// Extracts all stored surface data
        /// This is useful for saving the texture to a file
        SharedTextureData getTextureData();
        /// Same as getTextureData()->writeToFile(filename)
        void writeToFile(std::string const& filename);

    private:
        GLint previousTexture;              ///< previously bound tex
        BoundTextureRectangle* previousTexturePtr; ///< previously bound tex
        BoundTextureRectangle (TextureRectangle* buffer);
        friend class TextureRectangle;

        /// returns true iff it's safe to use this bound class
        /// otherwise, runtime error
        bool isCurrent() const;

    public:
        BoundTextureRectangle (BoundTextureRectangle &&); // allow move
        ~BoundTextureRectangle ();
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
    TextureRectangle(GLenum internalFormat = GL_RGBA);
    TextureRectangle(Shape const& shape);

    /// Binds this texture.
    /// Unbinding is done when the returned object runs out of scope.
    GLOW_NODISCARD BoundTextureRectangle bind() { return {this}; }
public: // static construction
    /// Creates a rectangular texture with given width and height
    static SharedTextureRectangle create(int width = 1, int height = 1, GLenum internalFormat = GL_RGBA);
    /// Creates a rectangular texture from Shape
    static SharedTextureRectangle create(Shape const& shape);
    static SharedTextureRectangle create(SizeT size, GLenum internalFormat = GL_RGBA) { return create(size.width, size.height, internalFormat); }
    /// Creates a rectangular texture with given width and height which is storage immutable
    static SharedTextureRectangle createStorageImmutable(int width, int height, GLenum internalFormat);
    static SharedTextureRectangle createStorageImmutable(SizeT size, GLenum internalFormat) { return createStorageImmutable(size.width, size.height, internalFormat); }
    static SharedTextureRectangle createStorageImmutable(Shape const& shape);

    /// Creates a rectangular texture from file
    /// See TextureData::createFromFile for format documentation
    /// Uses preferred internal format
    static SharedTextureRectangle createFromFile(std::string const& filename, ColorSpace colorSpace);
    /// same as createFromFile but with custom internal format
    static SharedTextureRectangle createFromFile(std::string const& filename, GLenum internalFormat, ColorSpace colorSpace);

    /// Creates a rectangular texture from given data
    /// Uses preferred internal format
    static SharedTextureRectangle createFromData(SharedTextureData const& data);
    /// same as createFromData but with custom internal format
    static SharedTextureRectangle createFromData(SharedTextureData const& data, GLenum internalFormat);

    friend class Framebuffer;
};
}
