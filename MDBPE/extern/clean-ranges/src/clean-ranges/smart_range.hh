#pragma once

#include <type_traits>

#include <clean-core/enable_if.hh>
#include <clean-core/equal_to.hh>
#include <clean-core/forward.hh>
#include <clean-core/functors.hh>
#include <clean-core/is_range.hh>
#include <clean-core/iterator.hh>
#include <clean-core/move.hh>

namespace cr
{
/**
 * A smart_range is a wrapper around some container/range
 * and offers a variety of functional-inspired methods for working with collections
 *
 * NOTE: ContainerT can be a reference (making this a view)
 *                  or a value (making this owning)
 *       const-ness should be shown in ContainerT, not "const smart_range"
 *
 * IMPORTANT: to use the algorithms, either
 *            #include <clean-ranges/algorithms/MY_SPECIFIC_ALGO.hh> or
 *            #include <clean-ranges/algorithms.hh>
 *
 * TODO: how to extend smart_range? maybe via inheritance and CRTP?
 */
template <class ContainerT>
struct smart_range
{
    static constexpr bool is_owning = !std::is_lvalue_reference_v<ContainerT>;
    static constexpr bool is_view = std::is_lvalue_reference_v<ContainerT>;
    static constexpr bool is_readonly = std::is_const_v<std::remove_reference_t<ContainerT>>;
    static_assert(!std::is_rvalue_reference_v<ContainerT>, "cannot hold rvalue references");

    using element_t = decltype(*cc::begin(std::declval<ContainerT&>()));
    using decayed_element_t = std::decay_t<element_t>;

    constexpr smart_range(ContainerT c) : _container(cc::forward<ContainerT>(c)) {}

    // pass-through collection
public:
    [[nodiscard]] constexpr auto begin() { return cc::begin(_container); }
    [[nodiscard]] constexpr auto begin() const { return cc::begin(_container); }
    [[nodiscard]] constexpr auto end() { return cc::end(_container); }
    [[nodiscard]] constexpr auto end() const { return cc::end(_container); }
    [[nodiscard]] constexpr decltype(auto) container() & { return _container; }
    [[nodiscard]] constexpr decltype(auto) container() const& { return _container; }
    [[nodiscard]] constexpr decltype(auto) container() && { return cc::move(_container); }
    [[nodiscard]] constexpr decltype(auto) container() const&& { return _container; }

    // algorithms
public:
    [[nodiscard]] constexpr bool is_empty() const { return !(this->begin() != this->end()); }
    [[nodiscard]] constexpr bool is_non_empty() const { return this->begin() != this->end(); }

    template <class RhsRange, class Comp = cc::equal_to<void>>
    [[nodiscard]] constexpr bool is_equal_to(RhsRange const& range, Comp&& comp = {});
    template <class RhsRange, class Comp = cc::equal_to<void>>
    [[nodiscard]] constexpr bool is_not_equal_to(RhsRange const& range, Comp&& comp = {});

    template <class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto sum(MapF&& f = {});
    template <class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto average(MapF&& f = {});

    template <class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto min(MapF&& f = {});
    template <class KeyF>
    [[nodiscard]] constexpr auto min_by(KeyF&& key);
    template <class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto max(MapF&& f = {});
    template <class KeyF>
    [[nodiscard]] constexpr auto max_by(KeyF&& key);
    template <class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto minmax(MapF&& f = {});
    template <class KeyF>
    [[nodiscard]] constexpr auto minmax_by(KeyF&& key);

    template <class Op, class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto reduce(Op&& op, MapF&& f = {});

    [[nodiscard]] constexpr size_t count();
    template <class T>
    [[nodiscard]] constexpr size_t count(T const& v);
    template <class Predicate = cc::constant_function<true>>
    [[nodiscard]] constexpr size_t count_if(Predicate&& predicate = {});

    template <class Predicate = cc::constant_function<true>>
    [[nodiscard]] constexpr decltype(auto) single(Predicate&& predicate = {});

    [[nodiscard]] constexpr decltype(auto) element_at(size_t idx);
    template <class T>
    [[nodiscard]] constexpr decltype(auto) element_at_or(size_t idx, T&& value);

    [[nodiscard]] constexpr auto drop(size_t cnt);
    template <class Predicate>
    [[nodiscard]] constexpr auto drop_while(Predicate&& pred);
    template <class Predicate>
    [[nodiscard]] constexpr auto drop_while_not(Predicate&& pred);

    [[nodiscard]] constexpr auto take(size_t cnt);
    template <class Predicate>
    [[nodiscard]] constexpr auto take_while(Predicate&& pred);
    template <class Predicate>
    [[nodiscard]] constexpr auto take_while_not(Predicate&& pred);

    template <class Predicate>
    [[nodiscard]] constexpr auto where(Predicate&& pred);
    template <class Predicate>
    [[nodiscard]] constexpr auto where_not(Predicate&& pred);

    template <class MapF>
    [[nodiscard]] constexpr auto map(MapF&& f);
    template <class MapF, class Predicate>
    [[nodiscard]] constexpr auto map_where(MapF&& f, Predicate&& pred);
    template <class MapF, class Predicate>
    [[nodiscard]] constexpr auto map_where_not(MapF&& f, Predicate&& pred);
    template <class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto each(MapF&& f = {});
    template <class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto repeat(MapF&& f = {});
    [[nodiscard]] constexpr auto times(size_t cnt);
    template <class T>
    constexpr auto pad_with(T&& value);

    template <class... MapF>
    [[nodiscard]] constexpr auto flatmap(MapF&&... maps);

    template <class T>
    [[nodiscard]] constexpr auto cast_to();

    template <class T>
    constexpr void fill(T const& value);

    template <class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto indexed(MapF&& f = {});
    template <class Range, class... Ranges>
    [[nodiscard]] constexpr auto zip(Range&& range, Ranges&&... ranges);
    template <class Range, class... Ranges>
    [[nodiscard]] constexpr auto concat(Range&& range, Ranges&&... ranges);

    template <class Container, class MapF = cc::identity_function>
    constexpr void collect(Container& container, MapF&& f = {});
    template <template <class...> class ContainerTemplate, class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto to(MapF&& f = {});
    template <class Container, class MapF = cc::identity_function>
    [[nodiscard]] constexpr auto to(MapF&& f = {});

    template <class RhsRange>
    constexpr void copy_to(RhsRange&& range);
    template <class RhsRange>
    constexpr void copy_from(RhsRange&& range);
    template <class RhsRange>
    constexpr void move_to(RhsRange&& range);
    template <class RhsRange>
    constexpr void move_from(RhsRange&& range);

    // operators
public:
    template <class RhsRange, cc::enable_if<cc::is_any_range<RhsRange>> = true>
    constexpr bool operator==(RhsRange const& rhs) const
    {
        auto itA = cc::begin(_container);
        auto itB = cc::begin(rhs);
        auto endA = cc::end(_container);
        auto endB = cc::end(rhs);

        while (itA != endA && itB != endB)
        {
            if (*itA != *itB)
                return false; // found unequal element

            ++itA;
            ++itB;
        }

        if (itA != endA || itB != endB)
            return false; // one range is longer than the other

        return true;
    }
    template <class RhsRange, cc::enable_if<cc::is_any_range<RhsRange>> = true>
    constexpr bool operator!=(RhsRange const& rhs) const
    {
        return !this->operator==(rhs);
    }

private:
    ContainerT _container;
};
}
