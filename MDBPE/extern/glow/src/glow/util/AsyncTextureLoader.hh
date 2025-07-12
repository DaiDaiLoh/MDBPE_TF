#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <glow/gl.hh>

#include <glow/objects/Texture1D.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/Texture3D.hh>
#include <glow/objects/TextureCubeMap.hh>
#include <glow/objects/TextureRectangle.hh>

#include <glow/data/ColorSpace.hh>
#include <glow/data/TextureData.hh>

#include "AsyncTexture.hh"

namespace glow
{
/*
 * IMPORTANT: for now you must call AsyncTextureLoader::shutdown() manually if using async loading
 */
class AsyncTextureLoader
{
private:
    struct task
    {
        std::function<void()> load;

        // lifetime is coupled to capture of load
        bool* result_ready;
        std::chrono::time_point<std::chrono::system_clock> const* access_time;
    };

    void enqueue(std::function<void()> const& load, bool* result_ready, std::chrono::time_point<std::chrono::system_clock> const* access_time)
    {
        // enqueue task
        {
            std::lock_guard<std::mutex> lk(mQueueMutex);
            mQueue.push_back({load, result_ready, access_time});
        }

        // notify a thread
        mQueueHasUpdates.notify_one();
    }

public:
    template <class TextureT>
    static AsyncTexture<TextureT> load(std::string const& filename, ColorSpace colorSpace)
    {
        lazyInit();

        AsyncTexture<TextureT> tex;
        auto data = tex.mData;
        data->internalFormat = 0;
        sInstance->enqueue(
            [data, filename, colorSpace] {
                data->data = TextureData::createFromFile(filename, colorSpace);
                data->ready = true;
            },
            &data->ready, &data->lastAccess);

        return tex;
    }
    template <class TextureT>
    static AsyncTexture<TextureT> load(std::string const& filename, GLenum internalFormat, ColorSpace colorSpace)
    {
        lazyInit();

        AsyncTexture<TextureT> tex;
        auto data = tex.mData;
        data->internalFormat = internalFormat;
        sInstance->enqueue(
            [data, filename, colorSpace] {
                data->data = TextureData::createFromFile(filename, colorSpace);
                data->ready = true;
            },
            &data->ready, &data->lastAccess);

        return tex;
    }

#define ASYNC_TEXTURE_LOAD(Suffix)                                                                                      \
    static AsyncTexture##Suffix load##Suffix(std::string const& filename, GLenum internalFormat, ColorSpace colorSpace) \
    {                                                                                                                   \
        return load<Texture##Suffix>(filename, internalFormat, colorSpace);                                             \
    }                                                                                                                   \
    static AsyncTexture##Suffix load##Suffix(std::string const& filename, ColorSpace colorSpace)                        \
    {                                                                                                                   \
        return load<Texture##Suffix>(filename, colorSpace);                                                             \
    }

    ASYNC_TEXTURE_LOAD(1D);
    ASYNC_TEXTURE_LOAD(2D);
    ASYNC_TEXTURE_LOAD(3D);
    ASYNC_TEXTURE_LOAD(Rectangle);

    static AsyncTexture<TextureCubeMap> loadCube(std::string const& fpx,
                                                 std::string const& fnx,
                                                 std::string const& fpy,
                                                 std::string const& fny,
                                                 std::string const& fpz,
                                                 std::string const& fnz,
                                                 ColorSpace colorSpace)
    {
        lazyInit();

        AsyncTexture<TextureCubeMap> tex;
        auto data = tex.mData;
        data->internalFormat = 0;
        sInstance->enqueue(
            [data, fpx, fnx, fpy, fny, fpz, fnz, colorSpace] {
                data->data = TextureData::createFromFileCube(fpx, fnx, fpy, fny, fpz, fnz, colorSpace);
                data->ready = true;
            },
            &data->ready, &data->lastAccess);

        return tex;
    }

#undef ASYNC_TEXTURE_LOAD
public:
    static void shutdown();

private:
    static std::unique_ptr<AsyncTextureLoader> sInstance;
    static void lazyInit();

    // queue
    std::vector<task> mQueue;
    std::mutex mQueueMutex;
    std::condition_variable mQueueHasUpdates;

    // threads
    // (last due to initialization order)
    std::atomic_bool mIsRunning{true};
    std::thread mThread;

    void run_thread();

    void cleanup();

private:
    AsyncTextureLoader() : mThread([this] { run_thread(); }) {}
};
}
