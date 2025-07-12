#pragma once

#include <clean-core/iterator.hh>

namespace cr
{
/// returns true iff the range has no elements
template <class Range>
[[nodiscard]] constexpr bool is_empty(Range&& range)
{
    return !(cc::begin(range) != cc::end(range));
}

/// returns true iff the range has elements
template <class Range>
[[nodiscard]] constexpr bool is_non_empty(Range&& range)
{
    return cc::begin(range) != cc::end(range);
}
}
