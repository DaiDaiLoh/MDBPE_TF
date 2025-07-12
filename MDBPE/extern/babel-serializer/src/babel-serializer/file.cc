#include "file.hh"

#include <filesystem>
#include <fstream>

#if defined(CC_OS_WINDOWS)
// clang-format off
#include <clean-core/native/detail/win32_sanitize_before.inl>

#include <Windows.h>

#include <WinBase.h>
#include <fileapi.h>

#include <clean-core/native/detail/win32_sanitize_after.inl>
// clang-format on
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <clean-core/array.hh>
#include <clean-core/assertf.hh>
#include <clean-core/format.hh>
#include <clean-core/macros.hh>
#include <clean-core/string.hh>
#include <clean-core/temp_cstr.hh>

void babel::file::read(cc::stream_ref<std::byte> out, cc::string_view filename, error_handler on_error)
{
    std::ifstream file(cc::temp_cstr(filename), std::ios_base::binary);
    if (!file)
    {
        on_error({}, {}, cc::format("file '{}' could not be read", filename), severity::error);
        return;
    }

    std::byte buffer[1024 * 4];
    while (file)
    {
        auto n = file.readsome(reinterpret_cast<char*>(buffer), sizeof(buffer));
        out << cc::span(buffer, n);
    }
}

cc::string babel::file::read_all_text(cc::string_view filename, babel::error_handler on_error)
{
    std::ifstream file(cc::temp_cstr(filename), std::ios_base::binary);
    if (!file)
    {
        on_error({}, {}, cc::format("file '{}' could not be read", filename), severity::error);
        return "";
    }

    file.seekg(0, std::ios_base::end);
    auto size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    auto result = cc::string::uninitialized(size);
    file.read(result.data(), size);
    return result;
}

cc::alloc_array<char> babel::file::read_all_text(cc::string_view filename, cc::allocator* alloc, babel::error_handler on_error)
{
#ifdef CC_OS_WINDOWS
    std::FILE* fp = nullptr;
    errno_t err = ::fopen_s(&fp, cc::temp_cstr(filename), "rb");
    if (err != 0)
        fp = nullptr;
#else
    std::FILE* fp = std::fopen(cc::temp_cstr(filename), "rb");
#endif

    if (!fp)
    {
        on_error({}, {}, cc::format("file '{}' could not be read", filename), severity::error);
        return {};
    }

    std::fseek(fp, 0, SEEK_END);
    auto res = cc::alloc_array<char>::uninitialized(std::ftell(fp) + 1, alloc);
    std::rewind(fp);
    std::fread(&res[0], 1, res.size() - 1, fp);
    std::fclose(fp);
    res[res.size() - 1] = '\0';
    return res;
}

cc::array<std::byte> babel::file::read_all_bytes(cc::string_view filename, babel::error_handler on_error)
{
    std::ifstream file(cc::temp_cstr(filename), std::ios_base::binary);
    if (!file)
    {
        on_error({}, {}, cc::format("file '{}' could not be read", filename), severity::error);
        return {};
    }

    file.seekg(0, std::ios_base::end);
    auto size = file.tellg();
    file.seekg(0, std::ios_base::beg);

    auto result = cc::array<std::byte>::uninitialized(size);
    file.read(reinterpret_cast<char*>(result.data()), size);
    return result;
}

cc::alloc_array<std::byte> babel::file::read_all_bytes(cc::string_view filename, cc::allocator* alloc, babel::error_handler on_error)
{
#ifdef CC_OS_WINDOWS
    std::FILE* fp = nullptr;
    errno_t err = ::fopen_s(&fp, cc::temp_cstr(filename), "rb");
    if (err != 0)
        fp = nullptr;
#else
    std::FILE* fp = std::fopen(cc::temp_cstr(filename), "rb");
#endif

    if (!fp)
    {
        on_error({}, {}, cc::format("file '{}' could not be read", filename), severity::error);
        return {};
    }

    std::fseek(fp, 0, SEEK_END);
    auto res = cc::alloc_array<std::byte>::uninitialized(std::ftell(fp), alloc);
    std::rewind(fp);
    std::fread(&res[0], 1, res.size(), fp);
    std::fclose(fp);
    return res;
}

void babel::file::write(cc::string_view filename, cc::span<std::byte const> data, babel::error_handler on_error)
{
    std::ofstream file(cc::temp_cstr(filename), std::ios_base::binary);
    if (!file)
    {
        on_error({}, {}, cc::format("cannot write to file '{}'", filename), severity::error);
        return;
    }

    file.write(reinterpret_cast<char const*>(data.data()), data.size());

    if (!file.good())
        on_error({}, {}, cc::format("error writing to file '{}'", filename), severity::warning);
}

void babel::file::write(cc::string_view filename, cc::string_view data, babel::error_handler on_error)
{
    write(filename, cc::as_byte_span(data), on_error);
}

void babel::file::write_lines(cc::string_view filename, cc::range_ref<cc::string_view> lines, cc::string_view line_ending, babel::error_handler on_error)
{
    std::ofstream file(cc::temp_cstr(filename), std::ios_base::binary);
    if (!file)
    {
        on_error({}, {}, cc::format("cannot write to file '{}'", filename), severity::error);
        return;
    }

    auto first = true;
    lines.for_each(
        [&](cc::string_view line)
        {
            if (first)
                first = false;
            else if (!line_ending.empty())
                file.write(line_ending.data(), line_ending.size());

            if (!line.empty())
                file.write(line.data(), line.size());
        });

    if (!file.good())
        on_error({}, {}, cc::format("error writing to file '{}'", filename), severity::warning);
}

bool babel::file::exists(cc::string_view filename)
{
    // TODO: replace by filestream api
    return std::ifstream(cc::temp_cstr(filename)).good();
}

babel::file::file_output_stream::file_output_stream(cc::string_view filename)
{
#ifdef CC_OS_WINDOWS
    errno_t err = ::fopen_s(&_file, cc::temp_cstr(filename), "wb");
    if (err != 0)
        _file = nullptr;
#else
    _file = std::fopen(cc::temp_cstr(filename), "wb");
#endif
}

babel::file::detail::mmap_info babel::file::detail::impl_map_file_to_memory(cc::string_view filepath, bool is_readonly)
{
#if defined(CC_OS_WINDOWS)

    // todo: error reporting
    // todo: arbitrarily long paths, see: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea

    auto file_handle_access_flags = GENERIC_READ;
    if (!is_readonly)
        file_handle_access_flags |= GENERIC_WRITE;

    auto const file_handle = CreateFileA(cc::temp_cstr(filepath), file_handle_access_flags, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    CC_ASSERT(file_handle != INVALID_HANDLE_VALUE && "failed to open file");

    DWORD high = 0;
    DWORD low = GetFileSize(file_handle, &high);
    size_t byte_size = (size_t(high) << 32) + size_t(low);

    auto const file_mapping_protection_flags = is_readonly ? PAGE_READONLY : PAGE_READWRITE;
    auto const file_mapping_handle = CreateFileMappingA(file_handle, nullptr, file_mapping_protection_flags, 0, 0, nullptr);
    CC_ASSERT(file_mapping_handle != INVALID_HANDLE_VALUE && "failed to create file mapping");

    auto const file_view_access_flags = is_readonly ? FILE_MAP_READ : FILE_MAP_WRITE; // FILE_MAP_WRITE gives read/write access
    auto const file_view = MapViewOfFile(file_mapping_handle, file_view_access_flags, 0, 0, 0);
    CC_ASSERT(file_view != INVALID_HANDLE_VALUE && "failed to create file view");

    return {file_handle, file_mapping_handle, byte_size, file_view};
#else
    // TODO:
    // The man page for mmap says the following:
    // After the mmap() call has returned, the file descriptor, fd, can be closed immediately without invalidating the mapping.
    // Does that mean we can close the file (even in read/write mode) after creating the map and still keep writing to it?
    // Is this even something we want?

    auto const file_access_flags = is_readonly ? O_RDONLY : O_RDWR;

    int const file_descriptor = open(cc::temp_cstr(filepath), file_access_flags);

    CC_ASSERT(file_descriptor != -1 && "failed to open file");

    struct stat st;
    auto const stat_error = fstat(file_descriptor, &st);
    CC_ASSERT(stat_error == 0 && "failed to read file size");
    size_t const byte_size = st.st_size;

    auto mmap_protection_flags = PROT_READ;
    if (!is_readonly)
        mmap_protection_flags |= PROT_WRITE;

    auto const mmap_flags = is_readonly ? MAP_PRIVATE : MAP_SHARED;

    void* const mmap_result = mmap(nullptr, byte_size, mmap_protection_flags, mmap_flags, file_descriptor, 0);
    CC_ASSERTF(mmap_result != MAP_FAILED, "Failed to map file to memory {}", filepath);

    return {file_descriptor, byte_size, mmap_result};
#endif
}

#if defined(CC_OS_WINDOWS)
void babel::file::detail::impl_unmap(HANDLE file_handle, HANDLE file_mapping_handle, void* file_view)
{
    if (file_view && file_view != INVALID_HANDLE_VALUE)
    {
        if (!UnmapViewOfFile(file_view))
            RICH_LOG_WARN("unable to close memory mapped file (UnmapViewOfFile)");
    }
    if (file_mapping_handle && file_mapping_handle != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(file_mapping_handle))
            RICH_LOG_WARN("unable to close memory mapped file (CloseHandle of mapping)");
    }
    if (file_handle && file_handle != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(file_handle))
            RICH_LOG_WARN("unable to close memory mapped file (CloseHandle of file)");
    }
}
#else
void babel::file::detail::impl_unmap(void* data, size_t size, int file_descriptor)
{
    if (data)
        munmap(data, size);
    if (file_descriptor != -1)
        close(file_descriptor);
}
#endif

babel::file::memory_mapped_file<std::byte> babel::file::make_memory_mapped_file_readwrite(cc::string_view path)
{
    return memory_mapped_file<std::byte>(path);
}

babel::file::memory_mapped_file<std::byte const> babel::file::make_memory_mapped_file_readonly(cc::string_view path)
{
    return memory_mapped_file<std::byte const>(path);
}

size_t babel::file::size_of(cc::string_view filename) { return std::filesystem::file_size(cc::string(filename).c_str()); }
