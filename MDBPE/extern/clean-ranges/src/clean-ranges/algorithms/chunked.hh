#pragma once

#include <clean-core/iterator.hh>
#include <clean-core/move.hh>
#include <clean-core/sentinel.hh>
#include <clean-core/vector.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
template <class Range, class MapF = cc::identity_function>
[[nodiscard]] constexpr auto chunked(Range&& range, size_t size, MapF f = {})
{
}
}