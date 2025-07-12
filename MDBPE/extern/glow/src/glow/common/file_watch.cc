#include "file_watch.hh"

#include <algorithm>
#include <array>
#include <codecvt>
#include <locale>
#include <mutex>
#include <thread>

#ifdef __unix__
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#endif // __unix__

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <tchar.h>
#include <cstdio>
#include <cstdlib>

#include <PathCch.h>
#include <Shlwapi.h>
#endif // WIN32

#include "non_copyable.hh"
#include "runtime_assert.hh"

namespace
{
static std::size_t constexpr bufferSize = 1024 * 256;


#ifdef _WIN32
DWORD const sListenFilters = FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_LAST_WRITE
                             | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME;

#elif defined __unix__
std::size_t constexpr sEventSize = (sizeof(struct inotify_event));
#endif
}

namespace glow
{
struct FileWatch::Monitor
{
public:
    struct FileLocation
    {
        std::string directory;
        std::string filename;

        FileLocation(std::string const& directory, std::string const& filename) : directory(directory), filename(filename) {}
    };

    static const FileLocation getFileLocation(const std::string_view path)
    {
        const auto predict = [](char character) -> bool {
#ifdef _WIN32
            return character == _T('\\') || character == _T('/');
#elif __unix__ || __APPLE__
            return character == '/';
#endif // __unix__
        };

        const auto pivot = std::find_if(path.rbegin(), path.rend(), predict).base();
        // if the path is something like "test.txt" there will be no directoy part, however we still need one, so insert './'
        const std::string directory = [&]() {
            const auto extracted_directory = std::string(path.begin(), pivot);
            return (extracted_directory.size() > 0) ? extracted_directory : "./";
        }();
        const std::string filename = std::string(pivot, path.end());
        return FileLocation(directory, filename);
    }

    struct FileEntry
    {
        FileLocation path;
        std::weak_ptr<Flag> weakPtr;
        Flag* rawPtr;

        FileEntry(FileLocation const& path, std::weak_ptr<Flag> weak, Flag* ptr) : path(path), weakPtr(weak), rawPtr(ptr) {}
    };

private:
    GLOW_NON_COPYABLE(Monitor);

    std::string path; // the path watched by this monitor

    std::mutex fileMutex;
    std::vector<FileEntry> files; // the files relevant to this monitor

    std::thread thread;
    std::atomic_bool threadAlive = {true};

    // == platform specifics ==
#ifdef __unix__
    int unixFolder;
    int unixWatch;
#elif defined _WIN32
    HANDLE winCloseEvent;
    HANDLE winDirectory;
#endif

private:
    void launch()
    {
        thread = std::thread([this]() {

        // == Native Setup ==
#if defined __unix__
            std::array<char, bufferSize> buffer;
#elif defined _WIN32
            std::array<BYTE, bufferSize> buffer;
            DWORD bytesReturned = 0;
            OVERLAPPED overlappedBuffer;
            overlappedBuffer.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
            GLOW_RUNTIME_ASSERT(!!overlappedBuffer.hEvent, "Failed to create Win32 monitor event", return );

            std::array<HANDLE, 2> handles = {overlappedBuffer.hEvent, this->winCloseEvent};
            auto asyncPending = false;
#endif

            while (this->threadAlive.load(std::memory_order_acquire))
            {
                // == Native Loop Body ==
#ifdef __unix__
                auto const length = read(this->unixFolder, static_cast<void*>(buffer.data()), buffer.size());

                if (length > 0)
                {
                    int i = 0;
                    while (i < length)
                    {
                        inotify_event* event = reinterpret_cast<inotify_event*>(&buffer[size_t(i)]);
                        if (event->len /*&& (event->mask & IN_CREATE || event->mask & IN_MODIFY)*/)
                        {
                            FileLocation changedFile = getFileLocation(std::string(event->name));

                            // Loop over every registered file
                            {
                                std::lock_guard<std::mutex> lock(this->fileMutex);
                                for (auto const& file : this->files)
                                    if (changedFile.filename == file.path.filename)
                                    {
                                        file.rawPtr->mChanged.store(true, std::memory_order_release);
                                        break;
                                    }
                            }
                        }
                        i += sEventSize + event->len;
                    }
                }

#elif defined _WIN32
                ReadDirectoryChangesW(this->winDirectory, buffer.data(), static_cast<DWORD>(buffer.size()), TRUE, sListenFilters, &bytesReturned,
                                      &overlappedBuffer, nullptr);

                asyncPending = true;

                auto res = WaitForMultipleObjects(2, handles.data(), FALSE, INFINITE);

                if (res == WAIT_OBJECT_0)
                {
                    auto overlapResultSuccess = GetOverlappedResult(this->winDirectory, &overlappedBuffer, &bytesReturned, TRUE);
                    GLOW_RUNTIME_ASSERT(overlapResultSuccess, "Call to GetOverlappedResult failed", continue);
                    asyncPending = false;

                    if (bytesReturned == 0)
                        continue;

                    FILE_NOTIFY_INFORMATION* fileInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buffer[0]);
                    if (fileInfo->Action == FILE_ACTION_MODIFIED)
                    {
                        // Loop over every entry in the fileInfo
                        for (;;)
                        {
                            FileLocation changedFile = getFileLocation(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(
                                std::wstring(fileInfo->FileName, fileInfo->FileNameLength / 2)));

                            // Loop over every registered file
                            {
                                std::lock_guard<std::mutex> lock(this->fileMutex);
                                for (auto const& file : this->files)
                                    if (changedFile.filename == file.path.filename)
                                    {
                                        file.rawPtr->mChanged.store(true, std::memory_order_release);
                                        // break;
                                    }
                            }

                            if (fileInfo->NextEntryOffset == 0)
                                break;

                            fileInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(fileInfo) + fileInfo->NextEntryOffset);
                        }
                    }
                }
#endif
            }

            // == Native Cleanup ==
#ifdef __unix__
            close(this->unixFolder);
#elif defined _WIN32
            if (asyncPending)
            {
                CancelIo(this->winDirectory);
                GetOverlappedResult(this->winDirectory, &overlappedBuffer, &bytesReturned, TRUE);
            }

            CloseHandle(this->winDirectory);
#endif
        });
    }

public:
#ifdef __unix__
    Monitor(std::string path, int folder, int watch) : path(std::move(path)), unixFolder(folder), unixWatch(watch) { launch(); }
#elif defined _WIN32
    Monitor(std::string path, HANDLE close, HANDLE dir) : path(std::move(path)), winCloseEvent(close), winDirectory(dir) { launch(); }
#endif

    static std::unique_ptr<Monitor> create(std::string const& path)
    {
#ifdef __unix__
        auto unixFolder = inotify_init();
        GLOW_RUNTIME_ASSERT(unixFolder >= 0, "Failed to initialize inotify", return nullptr);
        auto unixWatch = inotify_add_watch(unixFolder, path.c_str(), IN_MODIFY | IN_CREATE);
        GLOW_RUNTIME_ASSERT(unixWatch >= 0, "Failed to create inotify watch", return nullptr);

        return std::make_unique<Monitor>(path, unixFolder, unixWatch);
#elif defined _WIN32
        auto winCloseEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        GLOW_RUNTIME_ASSERT(!!winCloseEvent, "Failed to create Win32 close event", return nullptr);
        auto winDirectory = ::CreateFile(path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                         OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
        GLOW_RUNTIME_ASSERT(winDirectory != INVALID_HANDLE_VALUE, "Failed to create Win32 directory handle", return nullptr);

        return std::make_unique<Monitor>(path, winCloseEvent, winDirectory);
#elif defined __APPLE__
        return {};
#endif
    }

    ~Monitor()
    {
        threadAlive.store(false, std::memory_order_release);
#ifdef __unix__
        if (inotify_rm_watch(unixFolder, unixWatch) == -1)
            error() << "Failed to remove inotify watch";
#elif defined _WIN32
        SetEvent(winCloseEvent);
#endif
        thread.join();
    }

    // add a file to the watchlist if the monitor can accomodate it
    bool add(FileEntry const& file)
    {
        if (file.path.directory == path)
        {
            // This file is inside this folder (top-level), add it to the list
            std::lock_guard<std::mutex> lock(fileMutex);
            files.push_back(file);
            return true;
        }
        else
        {
            return false;
        }
    }

    // removes a file to watch if on the watchlist
    bool remove(Flag* flag)
    {
        for (auto it = files.begin(); it != files.end(); ++it)
            if (it->rawPtr == flag)
            {
                std::lock_guard<std::mutex> lock(fileMutex);
                files.erase(it);
                return true;
            }

        return false;
    }

    bool isEmpty() const { return files.empty(); }

    // Returns the the locked flag for a given filename, or nullptr if this monitor does not contain it
    SharedFlag getFlag(FileLocation const& path)
    {
        for (auto it = files.begin(); it != files.end(); ++it)
        {
            if (it->path.directory == path.directory && it->path.filename == path.filename)
            {
                info() << "Aliased " << path.filename;
                std::lock_guard<std::mutex> lock(fileMutex);
                return it->weakPtr.lock(); // Any weak_ptr in files is not expired
            }
        }

        return nullptr;
    }
};

FileWatch::Flag::~Flag() { FileWatch::onFlagDestruction(this); }

}

std::vector<std::unique_ptr<glow::FileWatch::Monitor>> glow::FileWatch::sMonitors;

void glow::FileWatch::onFlagDestruction(glow::FileWatch::Flag* flag)
{
    // Erase the flag from its monitor
    // TODO: We could figure out which monitor is responsible instead of brute-forcing here
    for (auto it = sMonitors.begin(); it != sMonitors.end(); ++it)
    {
        auto const& mon = *it;
        if (mon->remove(flag))
        {
            // If this was the last file of that monitor, destroy it
            if (mon->isEmpty())
                sMonitors.erase(it);

            // This is a search, the file is only part of a single monitor
            break;
        }
    }
}

glow::FileWatch::SharedFlag glow::FileWatch::watchFile(std::string_view filename, bool forceUnique)
{
    auto path = Monitor::getFileLocation(filename);

    if (!forceUnique)
    {
        // Check if the file already exists, if yes return an aliased shared_ptr
        for (auto const& monitor : sMonitors)
        {
            auto flag = monitor->getFlag(path);
            if (flag)
                return flag;
        }
    }

    // Create a new watch
    {
        auto watcher = std::make_shared<Flag>();
        Monitor::FileEntry entry = {path, watcher, watcher.get()};

        // See if any monitor is compatible
        bool foundExistingMonitor = false;
        for (auto const& mon : sMonitors)
        {
            if (mon->add(entry))
            {
                foundExistingMonitor = true;
                break;
            }
        }

        // Create a new monitor
        if (!foundExistingMonitor)
        {
            auto newMonitor = Monitor::create(entry.path.directory);
            if (!newMonitor)
            {
                glow::warning() << "Failed to watch " << filename << " for hotreloading";
#ifdef __unix__
                glow::warning() << "Consider increasing inotify watch limits: ";
                glow::warning() << "$ echo 16384 | sudo tee /proc/sys/fs/inotify/max_user_watches";
#endif
            }
            else
            {
                auto success = newMonitor->add(entry);
                // This should never fail
                GLOW_RUNTIME_ASSERT(success, "Fatal: Freshly created monitor cannot accomodate provoking file", );
                sMonitors.push_back(std::move(newMonitor));
            }
        }

        return watcher;
    }
}
