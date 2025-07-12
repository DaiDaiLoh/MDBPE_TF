#include <nexus/test.hh>

#include <clean-core/array.hh>
#include <clean-core/vector.hh>

#include <clean-ranges/algorithms/flatmap.hh>

TEST("cr::flatmap", disabled)
{
    struct bar
    {
        int nr = -1;
        int vals[3];

        explicit bar(int i)
        {
            nr = i;
            vals[0] = i;
            vals[1] = i + 1;
            vals[2] = i + 2;
        }
    };
    struct foo
    {
        cc::vector<bar> bars;
    };

    cc::array<foo, 2> foos;
    foos[0].bars.emplace_back(2);
    foos[0].bars.emplace_back(7);
    foos[1].bars.emplace_back(-3);

    cc::vector<cc::array<int>> vals;
    vals.push_back({3, 1, 2});
    vals.push_back({1});
    vals.push_back({});
    vals.push_back({8, 7});

    // TODO
    // CHECK(cr::flatmap(vals) == cc::vector{3, 1, 2, 1, 8, 7});
}
