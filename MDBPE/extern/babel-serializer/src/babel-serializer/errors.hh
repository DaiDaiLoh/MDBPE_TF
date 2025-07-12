#pragma once

#include <clean-core/function_ref.hh>
#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

namespace babel
{
enum class severity
{
    warning,
    error
    // TODO: fatal_error?
};

/// called when a serialization error occured
/// NOTE: this function is allowed to throw exceptions through library code!
using error_handler = cc::function_ref<void(cc::span<std::byte const> data, cc::span<std::byte const> pos, cc::string_view message, severity)>;

/// The defaul error handler outputs all warnings and errors on the console (using rich-log)
/// and asserts on error
///
/// TODO: make an object that caches source_map if called multiple times
void default_error_handler(cc::span<std::byte const> data, cc::span<std::byte const> pos, cc::string_view message, severity s);
}
