#pragma once

#include <chrono>

#include <glow/gl.hh>

#include <glow/fwd.hh>

namespace glow
{
/**
 * Usage:
 *
 *  AsyncTexture2D tex = AsyncTextureLoader::load<Texture2D>(filename, colorSpace);
 *  AsyncTexture2D tex = AsyncTextureLoader::load2D(filename, colorSpace);
 *
 *  if (tex)
 *  {
 *      shader.setTexture("uTexture", tex.texture());
 *      .. draw
 *  }
 *
 * TODO: bump priorities
 * TODO: share texture in data so that copies of the same async texture are only uploaded once
 */
struct AsyncTextureBase
{
    virtual std::shared_ptr<Texture> queryTexture() const = 0;

    virtual ~AsyncTextureBase() = default;
};
template <class TextureT>
struct AsyncTexture final : AsyncTextureBase
{
public:
    std::shared_ptr<Texture> queryTexture() const override { return std::dynamic_pointer_cast<Texture>(texture()); }

    std::shared_ptr<TextureT> const& texture() const
    {
        if (mTexture)
            return mTexture;

        if (operator bool())
        {
            auto fmt = mData->internalFormat;
            mTexture = fmt == 0 ? TextureT::createFromData(mData->data) : TextureT::createFromData(mData->data, fmt);
            mData = nullptr; // free memory
            return mTexture;
        }

        static std::shared_ptr<TextureT> null_tex = nullptr;
        return null_tex;
    }

    operator bool() const
    {
        if (mTexture)
            return true;

        if (mData->ready)
            return true;

        mData->lastAccess = std::chrono::system_clock::now();
        return false;
    }

    operator std::shared_ptr<TextureT> const&() const { return texture(); }

    AsyncTexture() = default;
    AsyncTexture(std::shared_ptr<TextureT> tex) { mTexture = tex; }

private:
    struct tdata
    {
        bool ready = false;
        std::chrono::time_point<std::chrono::system_clock> lastAccess;
        SharedTextureData data;
        GLenum internalFormat; // 0 means none
    };

    mutable std::shared_ptr<TextureT> mTexture;
    mutable std::shared_ptr<tdata> mData = std::make_shared<tdata>();

    friend class AsyncTextureLoader;
};
}
