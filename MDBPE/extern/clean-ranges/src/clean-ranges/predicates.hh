#pragma once

#include <clean-core/macros.hh>

// common usable predicates as functor objects
// NOTE: all arguments must survive the predicate!

namespace cr
{
// var is called "v"
#define CR_PREDICATE(expr)                          \
    constexpr struct                                \
    {                                               \
        template <class T>                          \
        constexpr auto operator()(T const& v) const \
        {                                           \
            return expr;                            \
        }                                           \
    }

#define CR_DEFINE_LHS_RHS_PREDICATE(name, expr)          \
    template <class T>                                   \
    constexpr auto name(T const& rhs)                    \
    {                                                    \
        return [&rhs](auto const& lhs) { return expr; }; \
    }                                                    \
    CC_FORCE_SEMICOLON

CR_PREDICATE(v > 0) is_positive;
CR_PREDICATE(v < 0) is_negative;
CR_PREDICATE(v >= 0) is_non_negative;
CR_PREDICATE(v <= 0) is_non_positive;
CR_PREDICATE(v == 0) is_zero;
CR_PREDICATE(v != 0) is_non_zero;

CR_PREDICATE(bool(v)) is_true;
CR_PREDICATE(!bool(v)) is_false;

CR_PREDICATE(v == nullptr) is_null;
CR_PREDICATE(v != nullptr) is_non_null;

CR_DEFINE_LHS_RHS_PREDICATE(is_equal_to, lhs == rhs);
CR_DEFINE_LHS_RHS_PREDICATE(is_not_equal_to, lhs != rhs);
CR_DEFINE_LHS_RHS_PREDICATE(is_greater_than, lhs > rhs);
CR_DEFINE_LHS_RHS_PREDICATE(is_greater_or_equal_to, lhs >= rhs);
CR_DEFINE_LHS_RHS_PREDICATE(is_smaller_than, lhs < rhs);
CR_DEFINE_LHS_RHS_PREDICATE(is_smaller_or_equal_to, lhs <= rhs);
}
