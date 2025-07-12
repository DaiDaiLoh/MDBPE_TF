#include <nexus/test.hh>

#include <clean-core/vector.hh>

#include <clean-ranges/algorithms/each.hh>
#include <clean-ranges/algorithms/map.hh>
#include <clean-ranges/range.hh>

TEST("cr::each")
{
    cc::vector<int> v;

    //
    // op=
    //

    v = {3, 1, 2};
    cr::each(v) += 2;
    CHECK(v == cc::vector{5, 3, 4});

    v = {3, 1, 2};
    cr::each(v) -= 2;
    CHECK(v == cc::vector{1, -1, 0});

    v = {3, 1, 2};
    cr::each(v) *= 2;
    CHECK(v == cc::vector{6, 2, 4});

    v = {3, 1, 2};
    cr::each(v) /= 2;
    CHECK(v == cc::vector{1, 0, 1});

    v = {3, 1, 2};
    cr::each(v) <<= 2;
    CHECK(v == cc::vector{12, 4, 8});

    v = {3, 1, 2};
    cr::each(v) >>= 1;
    CHECK(v == cc::vector{1, 0, 1});

    v = {3, 1, 2};
    cr::each(v) ^= 1;
    CHECK(v == cc::vector{2, 0, 3});

    v = {3, 1, 2};
    cr::each(v) &= 1;
    CHECK(v == cc::vector{1, 1, 0});

    v = {3, 1, 2};
    cr::each(v) |= 2;
    CHECK(v == cc::vector{3, 3, 2});

    v = {3, 1, 2};
    cr::each(v) %= 3;
    CHECK(v == cc::vector{0, 1, 2});

    v = {3, 1, 2};
    cr::each(v) += cr::inf_range(1);
    CHECK(v == cc::vector{4, 3, 5});

    v = {3, 1, 2};
    cr::each(v) += {2, 0, 1, 4};
    CHECK(v == cc::vector{5, 1, 3});

    //
    // op
    //

    v = {3, 1, 2};
    CHECK(cr::each(v) + 2 == cc::vector{5, 3, 4});
    CHECK(cr::each(v) - 2 == cc::vector{1, -1, 0});
    CHECK(cr::each(v) * 2 == cc::vector{6, 2, 4});
    CHECK(cr::each(v) / 2 == cc::vector{1, 0, 1});

    //
    // mapped
    //

    struct foo
    {
        int v;
    };
    cc::vector<foo> fv;
    fv.push_back({2});
    fv.push_back({3});
    cr::each(fv, &foo::v) += 5;
    CHECK(cr::map(fv, &foo::v) == cc::vector{7, 8});
    ++cr::each(fv, &foo::v);
    CHECK(cr::map(fv, &foo::v) == cc::vector{8, 9});
}
