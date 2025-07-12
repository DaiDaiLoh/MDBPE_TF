#include <nexus/test.hh>

#include <clean-core/array.hh>
#include <clean-core/vector.hh>

#include <clean-ranges/algorithms.hh>
#include <clean-ranges/range.hh>

TEST("generalized call")
{
    struct foo
    {
        int x;
        bool is_one() const { return x == 1; }
    };
    cc::vector<foo> v;
    v.push_back({3});
    v.push_back({1});
    v.push_back({2});

    CHECK(cr::sum(v, &foo::x) == 6);
    CHECK(cr::single(v, &foo::is_one).x == 1);
    CHECK(cr::first(v, &foo::is_one).x == 1);
    CHECK(cr::last(v, &foo::is_one).x == 1);
    CHECK(cr::any(v, &foo::is_one));
    CHECK(cr::count_if(v, &foo::is_one) == 1);
    CHECK(cr::min(v, &foo::x) == 1);
    CHECK(cr::max(v, &foo::x) == 3);
    CHECK(cr::minmax(v, &foo::x).max == 3);
    CHECK(cr::min_by(v, &foo::x).x == 1);
    CHECK(cr::max_by(v, &foo::x).x == 3);
    CHECK(cr::minmax_by(v, &foo::x).max.x == 3);
    CHECK(cr::index_of_first(v, &foo::is_one) == 1);
    CHECK(cr::index_of_last(v, &foo::is_one) == 1);
    CHECK(cr::index_of_first_safe(v, &foo::is_one) == 1);
    CHECK(cr::index_of_last_safe(v, &foo::is_one) == 1);
    CHECK(cr::find(v, &foo::is_one).x == 1);
    CHECK(cr::find_last(v, &foo::is_one).x == 1);
    CHECK(cr::find_or(v, &foo::is_one, foo{}).x == 1);
    CHECK(cr::find_last_or(v, &foo::is_one, foo{}).x == 1);
    CHECK(cr::average(v, &foo::x) == 2);
    CHECK(cr::average<float>(v, &foo::x) == 2);
    CHECK(cr::to<cc::vector>(v, &foo::x) == cc::vector{3, 1, 2});
    CHECK(cr::map(v, &foo::x).sum() == 6);
    CHECK(cr::map_where(v, &foo::x, &foo::is_one).single() == 1);
    CHECK(cr::map_where_not(v, &foo::x, &foo::is_one).sum() == 5);
    CHECK(cr::drop_while(v, &foo::is_one).sum(&foo::x) == 6);
    CHECK(cr::drop_while_not(v, &foo::is_one).sum(&foo::x) == 3);
    CHECK(cr::take_while(v, &foo::is_one).count() == 0);
    CHECK(cr::take_while_not(v, &foo::is_one).sum(&foo::x) == 3);
}
