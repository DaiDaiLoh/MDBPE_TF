#include <nexus/test.hh>

#include <clean-core/array.hh>
#include <clean-core/vector.hh>

#include <clean-ranges/algorithms.hh>
#include <clean-ranges/range.hh>

namespace
{
struct immovable_range
{
    cc::array<int> a = {1, 2, 3, 4, 5};
    auto begin() const { return a.begin(); }
    auto end() const { return a.end(); }

    immovable_range() = default;
    immovable_range(immovable_range const&) = delete;
    immovable_range(immovable_range&&) = delete;
    immovable_range& operator=(immovable_range const&) = delete;
    immovable_range& operator=(immovable_range&&) = delete;
};

struct immov_is_odd
{
    immov_is_odd() = default;
    immov_is_odd(immov_is_odd const&) = delete;
    immov_is_odd(immov_is_odd&&) = delete;
    immov_is_odd& operator=(immov_is_odd const&) = delete;
    immov_is_odd& operator=(immov_is_odd&&) = delete;

    bool operator()(int x) const { return x % 2 != 0; }
};

struct immov_plus1
{
    immov_plus1() = default;
    immov_plus1(immov_plus1 const&) = delete;
    immov_plus1(immov_plus1&&) = delete;
    immov_plus1& operator=(immov_plus1 const&) = delete;
    immov_plus1& operator=(immov_plus1&&) = delete;

    int operator()(int x) const { return x + 1; }
};

struct immov_idx_sum
{
    immov_idx_sum() = default;
    immov_idx_sum(immov_idx_sum const&) = delete;
    immov_idx_sum(immov_idx_sum&&) = delete;
    immov_idx_sum& operator=(immov_idx_sum const&) = delete;
    immov_idx_sum& operator=(immov_idx_sum&&) = delete;

    template <class T>
    int operator()(T const& v) const
    {
        return v.value + int(v.index);
    }
};
}

TEST("cr copy guarantees")
{
    immovable_range ir;

    immov_plus1 plus_one;
    immov_is_odd is_odd;
    immov_idx_sum idx_sum;

    CHECK(cr::range(ir) == cc::vector{1, 2, 3, 4, 5});
    CHECK(cr::range(ir).sum() == 15);
    CHECK(cr::range(ir).sum(plus_one) == 20);
    CHECK(cr::range(ir).map(plus_one).sum() == 20);
    CHECK(cr::range(ir).map(plus_one).where(is_odd).sum() == 3 + 5);
    CHECK(cr::range(ir).map(plus_one).where_not(is_odd).sum() == 2 + 4 + 6);
    CHECK(cr::range(ir).map_where(plus_one, is_odd).sum() == 2 + 4 + 6);
    CHECK(cr::range(ir).concat(ir) == cc::vector{1, 2, 3, 4, 5, 1, 2, 3, 4, 5});
    CHECK(cr::range(ir).where(is_odd).concat(ir) == cc::vector{1, 3, 5, 1, 2, 3, 4, 5});
    CHECK(cr::range(ir).where(is_odd).zip(ir).count() == 3);
    CHECK(cr::range(ir).drop(2) == cc::vector{3, 4, 5});
    CHECK(cr::range(ir).drop_while(is_odd) == cc::vector{2, 3, 4, 5});
    CHECK(cr::range(ir).drop_while_not(is_odd) == cc::vector{1, 2, 3, 4, 5});
    CHECK(cr::range(ir).take(2) == cc::vector{1, 2});
    CHECK(cr::range(ir).take_while(is_odd) == cc::vector{1});
    CHECK(cr::range(ir).take_while_not(is_odd) == cc::vector<int>{});
    CHECK(cr::range(ir).cast_to<double>() == cc::vector{1., 2., 3., 4., 5.});
    CHECK(cr::range(ir).repeat(plus_one).take(7) == cc::vector{2, 3, 4, 5, 6, 2, 3});
    CHECK(cr::range(ir).times(2) == cc::vector{1, 2, 3, 4, 5, 1, 2, 3, 4, 5});
    CHECK(cr::range(ir).indexed().map(idx_sum) == cc::vector{1, 3, 5, 7, 9});
}
