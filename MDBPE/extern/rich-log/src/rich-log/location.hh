#pragma once

#include <cstdint>
#include <ctime>

#include <clean-core/macros.hh>

namespace rlog
{
struct location
{
    // constant information about this location
    char const* const function;
    char const* const file;
    int const line;

    // runtime information for per-location features
    std::time_t last_log = {};
    bool break_on_log = false;
    bool break_on_log_once = false;
};
}
