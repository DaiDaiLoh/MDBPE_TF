#pragma once

#include <type_traits>

namespace cr::detail
{
/// returns A == void ? B : A
template <class A, class B>
using type_or = std::conditional_t<std::is_void_v<A>, B, A>;
}
