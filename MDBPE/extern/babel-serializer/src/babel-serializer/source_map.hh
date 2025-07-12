#pragma once

#include <clean-core/span.hh>
#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

namespace babel
{
/// creates a line/column source map from string or byte span
/// NOTE: - lines and columns are 0-based
///       - this is a "borrow" type and does NOT own the source
struct source_map
{
    source_map() = default;
    explicit source_map(cc::string_view source);
    explicit source_map(cc::span<std::byte const> source);

    cc::vector<cc::string_view> const& lines() const { return _lines; }

    /// returns 0-based line index of given char
    int line_of(char const* c) const;

    bool is_binary() const { return _is_binary; }

private:
    cc::string_view _source;
    // string views on all lines in the source
    cc::vector<cc::string_view> _lines;

    bool _is_binary = false;

    void parse(cc::string_view source);
};
}
