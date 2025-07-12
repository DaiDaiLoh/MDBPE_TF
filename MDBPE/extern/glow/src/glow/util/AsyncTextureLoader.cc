#include "AsyncTextureLoader.hh"

#include <glow/common/log.hh>

using namespace glow;

std::unique_ptr<AsyncTextureLoader> AsyncTextureLoader::sInstance;

void AsyncTextureLoader::lazyInit()
{
    if (sInstance)
        return;

    sInstance.reset(new AsyncTextureLoader);
}

void AsyncTextureLoader::run_thread()
{
    while (mIsRunning.load())
    {
        // getting a task
        std::unique_lock<std::mutex> lk(mQueueMutex);
        if (mQueue.empty())
            mQueueHasUpdates.wait(lk);
        if (mQueue.empty()) // wrong alarm OR shutting down
        {
            lk.unlock();
            continue;
        }

        // find best prio
        auto qs = mQueue.size();
        auto best_i = 0u;
        std::chrono::time_point<std::chrono::system_clock> best_access = *mQueue[best_i].access_time;
        for (auto i = 0u; i < qs; ++i)
        {
            auto const& task = mQueue[i];
            auto time = *task.access_time;

            if (time > best_access)
            {
                best_access = time;
                best_i = i;
            }
        }
        auto task = mQueue[best_i];
        std::swap(mQueue[best_i], mQueue.back());
        mQueue.pop_back();
        lk.unlock();

        // processing the task
        task.load();

        // notify another thread if applicable
        if (!mQueue.empty())
            mQueueHasUpdates.notify_one();
    }
}

void AsyncTextureLoader::shutdown()
{
    if (sInstance)
    {
        sInstance->cleanup();
        sInstance = nullptr;
    }
}

void AsyncTextureLoader::cleanup()
{
    mIsRunning.store(false);
    mQueueHasUpdates.notify_all();
    mThread.join();
}
