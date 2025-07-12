#include <nexus/test.hh>

#include <clean-core/set.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <algorithm>
#include <deque>
#include <list>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include <clean-ranges/algorithms/to.hh>

TEST("cr::collect")
{
    int v[] = {3, 1, 2};

    auto test_ordered = [](auto&& c) { CHECK(cr::to<cc::vector>(c) == cc::vector{3, 1, 2}); };
    auto test_unordered = [](auto&& c) {
        auto v = cr::to<cc::vector>(c);
        std::sort(v.begin(), v.end());
        CHECK(v == cc::vector{1, 2, 3});
    };

    // explicit
    test_ordered(cr::to<cc::vector<int>>(v));
    test_ordered(cr::to<std::vector<int>>(v));
    test_ordered(cr::to<std::deque<int>>(v));
    test_ordered(cr::to<std::list<int>>(v));
    test_unordered(cr::to<cc::set<int>>(v));
    test_unordered(cr::to<std::set<int>>(v));

    // implicit
    test_ordered(cr::to<cc::vector>(v));
    test_ordered(cr::to<std::vector>(v));
    test_ordered(cr::to<std::deque>(v));
    test_ordered(cr::to<std::list>(v));
    test_unordered(cr::to<cc::set>(v));
    test_unordered(cr::to<std::set>(v));

    // compile only
    CHECK(cr::to<std::stack<int>>(v).size() == 3);
    CHECK(cr::to<std::queue<int>>(v).size() == 3);
    CHECK(cr::to<std::stack>(v).size() == 3);
    CHECK(cr::to<std::queue>(v).size() == 3);

    // stream
    std::stringstream ss;
    cr::collect(v, ss);
    CHECK(ss.str() == "312");
    CHECK(cr::to<std::stringstream>(v).str() == "312");

    // string
    char cs[] = {'h', 'e', 'l', 'l', 'o'};
    CHECK(cr::to<cc::string>(cs) == "hello");
    CHECK(cr::to<std::string>(cs) == "hello");
}
