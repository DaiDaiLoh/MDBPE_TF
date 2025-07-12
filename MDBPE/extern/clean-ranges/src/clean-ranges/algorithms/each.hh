#pragma once

#include <initializer_list>

#include <clean-core/assert.hh>
#include <clean-core/iterator.hh>
#include <clean-core/sentinel.hh>

#include <clean-ranges/algorithms/map.hh>
#include <clean-ranges/detail/call.hh>
#include <clean-ranges/ranges/sentinel_range.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class ItT, class EndT, class MapF>
struct elementwise_iterator
{
    elementwise_iterator(ItT it, EndT end, MapF& mapF) : _it(cc::move(it)), _end(cc::move(end)), _mapF(mapF) {}

    constexpr decltype(auto) operator*() { return cr::detail::call(_idx, _mapF, *_it); }
    constexpr void operator++()
    {
        ++_it;
        ++_idx;
    }
    constexpr bool operator!=(cc::sentinel) const { return _it != _end; }

private:
    size_t _idx = 0;
    ItT _it;
    EndT _end;
    MapF& _mapF;
};

template <class Range, class MapF>
struct elementwise_range
{
    constexpr auto begin() { return elementwise_iterator(cc::begin(range), cc::end(range), mapF); }
    constexpr auto begin() const { return elementwise_iterator(cc::begin(range), cc::end(range), mapF); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    MapF mapF;

public:
    constexpr void operator++()
    {
        for (auto& v : *this)
            ++v;
    }
    constexpr void operator++(int)
    {
        for (auto& v : *this)
            v++;
    }
    constexpr void operator--()
    {
        for (auto& v : *this)
            --v;
    }
    constexpr void operator--(int)
    {
        for (auto& v : *this)
            v--;
    }

#define CR_IMPL_ELEMENTWISE_OPASSIGN(op)                               \
    template <class T>                                                 \
    constexpr void operator op(T&& rhs)                                \
    {                                                                  \
        if constexpr (cc::is_any_range<T>)                             \
        {                                                              \
            auto itA = this->begin();                                  \
            auto itB = cc::begin(rhs);                                 \
            auto endA = this->end();                                   \
            auto endB = cc::end(rhs);                                  \
                                                                       \
            while (itA != endA)                                        \
            {                                                          \
                CC_ASSERT(itB != endB && "second range is too short"); \
                                                                       \
                *itA op* itB;                                          \
                                                                       \
                ++itA;                                                 \
                ++itB;                                                 \
            }                                                          \
        }                                                              \
        else                                                           \
        {                                                              \
            for (auto& v : *this)                                      \
                v op rhs;                                              \
        }                                                              \
    }                                                                  \
    template <class T>                                                 \
    constexpr void operator op(std::initializer_list<T> rhs)           \
    {                                                                  \
        auto itA = this->begin();                                      \
        auto itB = rhs.begin();                                        \
        auto endA = this->end();                                       \
        auto endB = rhs.end();                                         \
                                                                       \
        while (itA != endA)                                            \
        {                                                              \
            CC_ASSERT(itB != endB && "second range is too short");     \
                                                                       \
            *itA op* itB;                                              \
                                                                       \
            ++itA;                                                     \
            ++itB;                                                     \
        }                                                              \
    }                                                                  \
    CC_FORCE_SEMICOLON

// TODO: support ranges for rhs
#define CR_IMPL_ELEMENTWISE_OP(op)                                                                           \
    template <class T>                                                                                       \
    constexpr auto operator op(T&& rhs) const                                                                \
    {                                                                                                        \
        return cr::map(cr::detail::sentinel_range{begin()}, [&rhs](auto const& lhs) { return lhs op rhs; }); \
    }                                                                                                        \
    CC_FORCE_SEMICOLON

    CR_IMPL_ELEMENTWISE_OP(+);
    CR_IMPL_ELEMENTWISE_OP(-);
    CR_IMPL_ELEMENTWISE_OP(*);
    CR_IMPL_ELEMENTWISE_OP(/);
    CR_IMPL_ELEMENTWISE_OP(%);
    CR_IMPL_ELEMENTWISE_OP(&);
    CR_IMPL_ELEMENTWISE_OP(|);
    CR_IMPL_ELEMENTWISE_OP(^);
    CR_IMPL_ELEMENTWISE_OP(<<);
    CR_IMPL_ELEMENTWISE_OP(>>);

    CR_IMPL_ELEMENTWISE_OPASSIGN(=);
    CR_IMPL_ELEMENTWISE_OPASSIGN(+=);
    CR_IMPL_ELEMENTWISE_OPASSIGN(-=);
    CR_IMPL_ELEMENTWISE_OPASSIGN(*=);
    CR_IMPL_ELEMENTWISE_OPASSIGN(/=);
    CR_IMPL_ELEMENTWISE_OPASSIGN(%=);
    CR_IMPL_ELEMENTWISE_OPASSIGN(&=);
    CR_IMPL_ELEMENTWISE_OPASSIGN(|=);
    CR_IMPL_ELEMENTWISE_OPASSIGN(^=);
    CR_IMPL_ELEMENTWISE_OPASSIGN(<<=);
    CR_IMPL_ELEMENTWISE_OPASSIGN(>>=);

#undef CR_IMPL_ELEMENTWISE_OP
#undef CR_IMPL_ELEMENTWISE_OPASSIGN
};
}

/// returns a special range that supports all operators on an element-wise basis (optionally on a mapped version of the original range)
/// e.g. cr::each(my_range) += 3;
///   or cr::each(my_range) = cr::each(my_range) * 3 + 2;
///   or cr::any(cr::each(my_range) == 7)
template <class Range, class MapF = cc::identity_function>
[[nodiscard]] constexpr auto each(Range&& range, MapF&& f = {})
{
    return detail::elementwise_range<Range, MapF>{cc::forward<Range>(range), cc::forward<MapF>(f)};
}

// [smart_range implementation]
template <class ContainerT>
template <class MapF>
constexpr auto smart_range<ContainerT>::each(MapF&& f)
{
    return cr::each(_container, cc::forward<MapF>(f));
}
}
