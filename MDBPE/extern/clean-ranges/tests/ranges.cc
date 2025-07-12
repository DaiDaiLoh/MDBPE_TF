#include <nexus/test.hh>

#include <clean-core/array.hh>
#include <clean-core/vector.hh>

#include <clean-ranges/range.hh>

TEST("cr range types")
{
    CHECK(cr::range({3, 1, 2}) == cc::vector{3, 1, 2});
    CHECK(cr::range(cc::array{3, 1, 2}) == cc::vector{3, 1, 2});
    CHECK(cr::range(4) == cc::vector{0, 1, 2, 3});
    CHECK(cr::range(2, 5) == cc::vector{2, 3, 4});
    CHECK(cr::range(2, 8, 2) == cc::vector{2, 4, 6});
    CHECK(cr::range(5, 2, -1) == cc::vector{5, 4, 3});
    CHECK(cr::range(1, 64, [](int x) { return x * 2; }) == cc::vector{1, 2, 4, 8, 16, 32});
    CHECK(cr::range(1, 64, [](int& x) { x = -x * 2; }) == cc::vector{1, -2, 4, -8, 16, -32});

    cc::vector<int> v = {3, 4, 2, 5};
    CHECK(cr::range(v.begin() + 1, v.end() - 1) == cc::vector{4, 2});

    struct my_range
    {
        int value = 3;
        int operator*() const { return value; }
        void operator++() { value += 2; }
        bool operator!=(cc::sentinel) const { return value <= 7; }
    };
    CHECK(cr::range(my_range{}) == cc::vector{3, 5, 7});
}
