#include "filesystem.hh"

#include <sys/stat.h>
#include <filesystem>

#include <clean-core/temp_cstr.hh>

void util::make_directories(cc::string_view path) { std::filesystem::create_directories(std::filesystem::path(path.begin(), path.end())); }

bool util::exists(cc::string_view path)
{
    struct stat buffer;
    return (stat(cc::temp_cstr(path), &buffer) == 0);
}
