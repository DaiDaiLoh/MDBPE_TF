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
GLOW_SHARED(class, Texture1DArray);
GLOW_SHARED(class, TextureData);

/// Defines an array of 1D textures in OpenGL
class Texture1DArray final : public Texture
{
public:
    struct BoundTexture1DArray;

    using SizeT = tg::isize2;

    struct Shape
    {
        GLenum format;
        SizeT size;
        int immutableMipmaps; ///< Amount of fixed mipmaps, -1 if the texture is not immutable. This parameter is only respected in ::createStorageImmutable

        inline bool operator==(Shape const& rhs) const
        {
            return (format == rhs.format) && (size == rhs.size) && (immutableMipmaps == rhs.immutableMipmaps);
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


    /// Level of anisotropic filtering (>= 1.f, which is isotropic)
    /// Max number of samples basically
    GLfloat mAnisotropicFiltering = 1.0f;

    /// Texture size: Width
    int mWidth = 0u;
    /// Texture size: Layers
    int mLayers = 0u;

    /// True iff mipmaps are generated since last data upload
    bool mMipmapsGenerated = false;

    /// LOD bias
    float mLodBias = 0.0f;
    /// Mipmap base level
    int mBaseLevel = 0;
    /// Mipmap max level
    int mMaxLevel = 1000;
    /// Mipmap min lod
    float mMinLod = -1000.0f;
    /// Mipmap max lod
    float mMaxLod = 1000.0f;


    /// if true, this texture got immutable storage by glTexStorage2D
    bool mStorageImmutable = false;
    /// Amount of fixed mipmaps, -1 if this texture does not have immutable storage
    int mImmutableMipmaps = -1;

public: // getter
    /// Gets the currently bound texture (nullptr if none)
    static BoundTexture1DArray* getCurrentTexture();

    GLenum getMinFilter() const { return mMinFilter; }
    GLenum getMagFilter() const { return mMagFilter; }
    tg::color4 getBorderColor() const { return mBorderColor; }
    GLenum getWrapS() const { return mWrapS; }
    int getWidth() const { return mWidth; }
    int getLayers() const { return mLayers; }
    tg::isize3 getDimensions() const override { return { mWidth, mLayers, 1 }; }
    SizeT getSize() const { return { mWidth, mLayers }; }
    
    Shape getShape() const { return { mInternalFormat, getSize(), mImmutableMipmaps }; }

    bool isStorageImmutable() const override { return mStorageImmutable; }

    /// returns true iff mipmaps are used (based on min filter)
    bool hasMipmapsEnabled() const override;
    /// returns true iff mipmaps were generated via bind().generateMipmaps() (and are still valid)
    bool areMipmapsGenerated() const override { return mMipmapsGenerated; }
    /// Manually sets the internal flag if mipmaps were generated
    /// CAUTION: this should only be used if you modified the mipmap status manually (e.g. via glGenerateMipmaps)
    void setMipmapsGenerated(bool areGenerated) override { mMipmapsGenerated = areGenerated; }

    float getLodBias() const { return mLodBias; }
    int getBaseLevel() const { return mBaseLevel; }
    int getMaxLevel() const { return mMaxLevel; }
    float getMinLod() const { return mMinLod; }
    float getMaxLod() const { return mMaxLod; }


    /// returns the uniform type that should be used for sampling this texture
    GLenum getUniformType() const override;

public:
    /// RAII-object that defines a "bind"-scope for an array of 1D textures
    /// All functions that operate on the currently bound tex are accessed here
    struct BoundTexture1DArray
    {
        GLOW_RAII_CLASS(BoundTexture1DArray);

        /// Backreference to the texture
        Texture1DArray* const texture;

        /// Makes the storage of this texture immutable
        /// It is an error to call this more than once
        /// It is an error to upload data with a different internal format at a later point
        /// It is an error to resize after storage was made immutable (unless it's the same size)
        /// Invalidates previously uploaded data
        /// If mipmapLevels is <= 0, log2(max(width, layers)) + 1 is used
        void makeStorageImmutable(int width, int layers, GLenum internalFormat, int mipmapLevels = 0);

        /// Sets minification filter (GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, ..., GL_LINEAR_MIPMAP_LINEAR)
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
        /// Sets texture wrapping in all directions
        void setWrap(GLenum wrapS);


        /// Generates mipmaps for this texture
        void generateMipmaps();

        /// Sets the bias (offset) for LOD
        void setLodBias(float bias);
        /// Sets the finest uploaded mip level
        void setBaseLevel(int lvl);
        /// Sets the coarsest uploaded mip level
        void setMaxLevel(int lvl);
        /// Sets the smallest lod value that should be used by texture(...)
        void setMinLod(float lvl);
        /// Sets the largest lod value that should be used by texture(...)
        void setMaxLod(float lvl);

        /// Resizes the texture
        /// invalidates the data
        void resize(int width, int layers);
        void resize(SizeT size) { resize(size.width, size.height); }


        /// Generic data uploads
        /// Changes internal format, width, layers, and data
        void setData(GLenum internalFormat, int width, int layers, GLenum format, GLenum type, const GLvoid* data, int mipmapLevel = 0);
        /// Data upload via glm, tg, or c++ type (see gltypeinfo)
        template <typename DataRangeT>
        void setData(GLenum internalFormat, int width, int layers, DataRangeT&& data, int mipmapLevel = 0)
        {
            static_assert(cc::is_any_contiguous_range<DataRangeT>, "data must be a contiguous range!");
            auto data_span = cc::span(data);
            using DataT = std::decay_t<decltype(data_span[0])>;
            static_assert(std::is_trivially_copyable_v<DataT>, "data must be trivially copyable");

            if (int(data_span.size()) != width * layers)
            {
                error() << "Texture size is " << width << " x " << layers << " = " << width * layers << " but " << data_span.size()
                        << " pixels are provided. " << to_string(texture);
                return;
            }
            setData(internalFormat, width, layers, glTypeOf<DataT>::format, glTypeOf<DataT>::type, data_span.data(), mipmapLevel);
        }
        /// same as above for initializer lists
        template <class DataT>
        void setData(GLenum internalFormat, int width, int layers, std::initializer_list<DataT> data, int mipmapLevel = 0)
        {
            setData(internalFormat, width, layers, cc::span<std::add_const_t<DataT>>(data.begin(), data.size()), mipmapLevel);
        }
        /// Same as above
        /// Usage:
        ///   tg::vec3 texData[][] = { ... }
        ///   // it's [layers][width]
        ///   setData(iFormat, texData);
        template <typename DataT, int width, int layers>
        void setData(GLenum internalFormat, const DataT(&data)[layers][width] , int mipmapLevel = 0)
        {
            setData(internalFormat, width, layers, glTypeOf<DataT>::format, glTypeOf<DataT>::type, data, mipmapLevel);
        }

        /// Generic partial data uploads
        /// Only changes data. Offset and size must be inside original bounds.
        void setSubData(int x, int l, int width, int layers, GLenum format, GLenum type, const GLvoid* data, int mipmapLevel = 0);
        /// Partial data upload via glm, tg, or c++ type (see gltypeinfo)
        template <typename DataRangeT>
        void setSubData(int x, int l, int width, int layers, DataRangeT&& data, int mipmapLevel = 0)
        {
            static_assert(cc::is_any_contiguous_range<DataRangeT>, "data must be a contiguous range!");
            auto data_span = cc::span(data);
            using DataT = std::decay_t<decltype(data_span[0])>;
            static_assert(std::is_trivially_copyable_v<DataT>, "data must be trivially copyable");

            if (int(data_span.size()) != width * layers)
            {
                error() << "Texture size is " << width << " x " << layers << " = " << width * layers << " but " << data_span.size()
                        << " pixels are provided. " << to_string(texture);
                return;
            }
            setSubData(x, l, width, layers, glTypeOf<DataT>::format, glTypeOf<DataT>::type, data_span.data(), mipmapLevel);
        }
        /// Same as above
        /// Usage:
        ///   tg::vec3 texData[][] = { ... }
        ///   // it's [layers][width]
        ///   setSubData(x, l, texData);
        template <typename DataT, int width, int layers>
        void setSubData(int x, int l, const DataT(&data)[layers][width] , int mipmapLevel = 0)
        {
            setSubData(x, l, width, layers, glTypeOf<DataT>::format, glTypeOf<DataT>::type, data, mipmapLevel);
        }

        /// Sets texture data from surface data
        /// May set multiple levels at once
        /// May modify texture parameter
        void setData(GLenum internalFormat, SharedTextureData const& data);

        /// Generic data download
        std::vector<char> getData(GLenum format, GLenum type, int mipmapLevel = 0);
        /// Generic data download
        void getData(GLenum format, GLenum type, size_t bufferSize, void* buffer, int mipmapLevel = 0);
        /// Data download via glm, tg, or c++ type (see gltypeinfo)
        template <typename DataT>
        std::vector<DataT> getData(int mipmapLevel = 0)
        {
            std::vector<DataT> data;
            data.resize(texture->mWidth * texture->mLayers);
            getData(glTypeOf<DataT>::format, glTypeOf<DataT>::type, data.size() * sizeof(DataT), data.data(), mipmapLevel);
            return std::move(data);
        }

        /* TODO: OpenGL <4.5 does not support subimage retrieval (in 4.5, https://www.opengl.org/sdk/docs/man/html/glGetTextureSubImage.xhtml can be used)
        /// Generic partial data download
        std::vector<char> getSubData(GLenum format, GLenum type, int x, int l, int width, int layers, int mipmapLevel = 0);
        /// Generic partial data download
        void getSubData(GLenum format, GLenum type, int x, int l, int width, int layers, size_t bufferSize, void* buffer, int mipmapLevel = 0);
        /// Partial data download via glm, tg, or c++ type (see gltypeinfo)
        template <typename DataT>
        std::vector<DataT> getSubData(int x, int l, int width, int layers, int mipmapLevel = 0)
        {
            std::vector<DataT> data;
            data.resize(width * layers);
            getSubData(glTypeOf<DataT>::format, glTypeOf<DataT>::type, x, l, width, layers, data.size() * sizeof(DataT), data.data(), mipmapLevel);
            return std::move(data);
        }
        */

        /// Extracts all stored surface data up to a given max mipmap level (inclusive)
        /// This is useful for saving the texture to a file
        SharedTextureData getTextureData(int maxMipLevel = 1000);
        /// Same as getTextureData()->writeToFile(filename)
        void writeToFile(std::string const& filename);

    private:
        GLint previousTexture;              ///< previously bound tex
        BoundTexture1DArray* previousTexturePtr; ///< previously bound tex
        BoundTexture1DArray (Texture1DArray* buffer);
        friend class Texture1DArray;

        /// returns true iff it's safe to use this bound class
        /// otherwise, runtime error
        bool isCurrent() const;

    public:
        BoundTexture1DArray (BoundTexture1DArray &&); // allow move
        ~BoundTexture1DArray ();
    };
public:

    /// Fills the specific mipmap level (default 0) with the given data
    /// Requires OpenGL 4.4 (for now) and will throw a run-time error otherwise
    void clear(GLenum format, GLenum type, const GLvoid* data, int mipmapLevel = 0);
    /// Clear via glm, tg, or c++ type (see gltypeinfo)
    /// CAREFUL: pointers do not work!
    template <typename DataT>
    void clear(DataT const& data, int mipmapLevel = 0)
    {
        clear(glTypeOf<DataT>::format, glTypeOf<DataT>::type, (const GLvoid*)&data, mipmapLevel);
    }

public:
    Texture1DArray(GLenum internalFormat = GL_RGBA);
    Texture1DArray(Shape const& shape);

    /// Binds this texture.
    /// Unbinding is done when the returned object runs out of scope.
    GLOW_NODISCARD BoundTexture1DArray bind() { return {this}; }
public: // static construction
    /// Creates an array of 1D textures with given width and height
    static SharedTexture1DArray create(int width = 1, int layers = 1, GLenum internalFormat = GL_RGBA);
    /// Creates an array of 1D textures from Shape
    static SharedTexture1DArray create(Shape const& shape);
    static SharedTexture1DArray create(SizeT size, GLenum internalFormat = GL_RGBA) { return create(size.width, size.height, internalFormat); }
    /// Creates an array of 1D textures with given width and height which is storage immutable
    /// If mipmapLevels is <= 0, log2(max(width, height)) + 1 is used
    static SharedTexture1DArray createStorageImmutable(int width, int layers, GLenum internalFormat, int mipmapLevels = 0);
    static SharedTexture1DArray createStorageImmutable(SizeT size, GLenum internalFormat, int mipmapLevels = 0) { return createStorageImmutable(size.width, size.height, internalFormat, mipmapLevels); }
    static SharedTexture1DArray createStorageImmutable(Shape const& shape);

    /// Creates an array of 1D textures from file
    /// See TextureData::createFromFile for format documentation
    /// Uses preferred internal format
    static SharedTexture1DArray createFromFile(std::string const& filename, ColorSpace colorSpace);
    /// same as createFromFile but with custom internal format
    static SharedTexture1DArray createFromFile(std::string const& filename, GLenum internalFormat, ColorSpace colorSpace);

    /// Creates an array of 1D textures from given data
    /// Uses preferred internal format
    static SharedTexture1DArray createFromData(SharedTextureData const& data);
    /// same as createFromData but with custom internal format
    static SharedTexture1DArray createFromData(SharedTextureData const& data, GLenum internalFormat);

    friend class Framebuffer;
};
}
