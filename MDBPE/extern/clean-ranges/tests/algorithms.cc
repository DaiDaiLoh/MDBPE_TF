#include <nexus/test.hh>

#include <clean-core/array.hh>
#include <clean-core/vector.hh>

#include <clean-ranges/algorithms.hh>
#include <clean-ranges/range.hh>

namespace
{
int plus_one(int x) { return x + 1; }

int prod(int a, int b) { return a * b; }

bool is_sqr_equal_sum(int x) { return x * x == x + x; }

bool is_odd(int x) { return x % 2 != 0; }
bool is_positive(int x) { return x > 0; }
bool is_negative(int x) { return x < 0; }

int mod3(int x) { return x % 3; }

template <class T>
struct show;
}

TEST("cr algorithms")
{
    std::initializer_list<int> empty = {};
    int v[] = {4, 3, 1, 2};
    cc::vector<int> v_eq = {4, 3, 1, 2};
    cc::array<int> v_uneq0 = {4, 5, 1, 2};
    cc::vector<int> v_uneq1 = {4, 3, 1};
    cc::vector<int> v_uneq2 = {4, 3, 1, 2, 6};

    auto const times_two = [](int x) { return x * 2; };

    CHECK(cr::sum(v) == 10);
    CHECK(cr::sum<float>(v) == 10.f);
    CHECK(cr::sum(v, plus_one) == 14);

    CHECK(cr::reduce(v, prod) == 4 * 3 * 1 * 2);
    CHECK(cr::reduce(v, prod, times_two) == 4 * 3 * 1 * 2 * 2 * 2 * 2 * 2);

    CHECK(cr::single(v, is_sqr_equal_sum) == 2);

    CHECK(cr::first(v) == 4);
    CHECK(cr::first(v, is_odd) == 3);

    CHECK(cr::last(v) == 2);
    CHECK(cr::last(v, is_odd) == 1);

    CHECK(!cr::any(empty));
    CHECK(cr::any(v));
    CHECK(cr::any(v, is_odd));
    CHECK(!cr::any(v, is_negative));

    CHECK(!cr::all(v, is_odd));
    CHECK(cr::all(v, is_positive));

    CHECK(cr::count(v) == 4);
    CHECK(cr::count(v, 3) == 1);
    CHECK(cr::count(v, -1) == 0);
    CHECK(cr::count_if(v, is_odd) == 2);

    CHECK(cr::contains(v, 1));
    CHECK(!cr::contains(v, -1));

    CHECK(cr::min(v) == 1);
    CHECK(cr::min(v, times_two) == 2);

    CHECK(cr::max(v) == 4);
    CHECK(cr::max(v, times_two) == 8);

    CHECK(cr::minmax(v).min == 1);
    CHECK(cr::minmax(v).max == 4);
    CHECK(cr::minmax(v, times_two).min == 2);
    CHECK(cr::minmax(v, times_two).max == 8);

    CHECK(cr::min_by(v, mod3) == 3);
    CHECK(cr::max_by(v, mod3) == 2);
    CHECK(cr::minmax_by(v, mod3).min == 3);
    CHECK(cr::minmax_by(v, mod3).max == 2);

    CHECK(!cr::is_empty(v));
    CHECK(cr::is_non_empty(v));
    CHECK(cr::is_empty(empty));
    CHECK(!cr::is_non_empty(empty));

    CHECK(cr::index_of(v, 3) == 1);
    CHECK(cr::index_of_safe(v, 1) == 2);
    CHECK(cr::index_of_safe(v, 5) == -1);

    CHECK(cr::index_of_first(v, is_odd) == 1);
    CHECK(cr::index_of_first_safe(v, is_odd) == 1);
    CHECK(cr::index_of_first_safe(v, is_negative) == -1);

    CHECK(cr::index_of_last(v, is_odd) == 2);
    CHECK(cr::index_of_last_safe(v, is_odd) == 2);
    CHECK(cr::index_of_last_safe(v, is_negative) == -1);

    CHECK(cr::find(v, is_odd) == 3);
    CHECK(cr::find_last(v, is_odd) == 1);
    CHECK(cr::find_or(v, is_odd, 7) == 3);
    CHECK(cr::find_or(v, is_negative, 7) == 7);
    CHECK(cr::find_last_or(v, is_odd, 7) == 1);
    CHECK(cr::find_last_or(v, is_negative, 7) == 7);

    CHECK(cr::element_at(v, 3) == 2);
    CHECK(cr::element_at_or(v, 2, 7) == 1);
    CHECK(cr::element_at_or(v, 8, 10) == 10);

    CHECK(cr::average(v) == 2);
    CHECK(cr::average<float>(v) == 2.5f);

    CHECK(cr::average(v, plus_one) == 3);
    CHECK(cr::average<float>(v, plus_one) == 3.5f);

    CHECK(cr::are_equal(v, v_eq));
    CHECK(!cr::are_equal(v, v_uneq0));
    CHECK(!cr::are_equal(v, v_uneq1));
    CHECK(!cr::are_equal(v, v_uneq2));

    CHECK(!cr::are_not_equal(v, v_eq));
    CHECK(cr::are_not_equal(v, v_uneq0));
    CHECK(cr::are_not_equal(v, v_uneq1));
    CHECK(cr::are_not_equal(v, v_uneq2));

    CHECK(cr::to<cc::vector>(v) == cc::vector{4, 3, 1, 2});
    CHECK(cr::to<cc::vector>(v, plus_one) == cc::vector{5, 4, 2, 3});

    cc::vector<int> coll;
    cr::collect(v, coll);
    CHECK(coll == cc::vector{4, 3, 1, 2});

    CHECK(cr::where(v, is_odd).sum() == 4);
    CHECK(cr::where_not(v, is_odd).sum() == 6);
    CHECK(cr::inf_range(10).where([](int x) { return x % 9 == 0; }).element_at(2) == 36);

    CHECK(cr::map(v, plus_one).sum() == 5 + 4 + 2 + 3);
    CHECK(cr::map(v, plus_one).where(is_odd).sum() == 5 + 3);
    CHECK(cr::map_where(v, plus_one, is_odd).sum() == 2 + 4);
    CHECK(cr::where(v, is_odd).map(plus_one).sum() == 2 + 4);
    CHECK(cr::range(3, 8).map(is_odd).to<cc::vector>() == cc::vector{true, false, true, false, true});

    CHECK(cr::range(0, 10).where(is_odd).map(plus_one).sum() == 2 + 4 + 6 + 8 + 10);

    CHECK(cr::average(v) == 2);
    CHECK(cr::cast_to<float>(v).average() == 2.5f);

    CHECK(cr::zip(v, v_uneq1).count() == 3);
    CHECK(cr::zip(v, v_uneq0).sum([](cc::tuple<int&, int&> v) { return v.get<0>() * v.get<1>(); }) == 4 * 4 + 3 * 5 + 1 * 1 + 2 * 2);

    CHECK(cr::concat(v, cr::range(0, 3)).to<cc::vector>() == cc::vector{4, 3, 1, 2, 0, 1, 2});
    CHECK(cr::concat(v, cr::range(0, 0)).to<cc::vector>() == cc::vector{4, 3, 1, 2});
    CHECK(cr::concat(v, cr::range(0, 0), cr::range(0, 3)).to<cc::vector>() == cc::vector{4, 3, 1, 2, 0, 1, 2});
    CHECK(cr::concat(cr::range(0, 0), v, cr::range(0, 3)).to<cc::vector>() == cc::vector{4, 3, 1, 2, 0, 1, 2});

    CHECK(!cr::range(v).is_empty());
    CHECK(cr::range(v).is_non_empty());

    CHECK(cr::drop(v, 2).sum() == 3);
    CHECK(cr::drop(v, 10).is_empty());
    CHECK(cr::drop(v, 2).to<cc::vector>() == cc::vector{1, 2});
    CHECK(cr::drop_while(v, is_odd).to<cc::vector>() == cc::vector{4, 3, 1, 2});
    CHECK(cr::drop_while(v, is_positive).to<cc::vector>() == cc::vector<int>());
    CHECK(cr::drop_while_not(v, is_odd).to<cc::vector>() == cc::vector{3, 1, 2});

    CHECK(cr::inf_range(3).take(5).to<cc::vector>() == cc::vector{3, 4, 5, 6, 7});
    CHECK(cr::inf_range(-3).take_while(is_negative).to<cc::vector>() == cc::vector{-3, -2, -1});
    CHECK(cr::inf_range(-3).take_while_not(is_positive).to<cc::vector>() == cc::vector{-3, -2, -1, 0});

    CHECK(cr::repeat(v).take(10) == cc::vector{4, 3, 1, 2, 4, 3, 1, 2, 4, 3});
    CHECK(cr::pad_with(v, 7).take(6) == cc::vector{4, 3, 1, 2, 7, 7});
    CHECK(cr::times(v, 0) == cc::vector<int>{});
    CHECK(cr::times(v, 1) == cc::vector{4, 3, 1, 2});
    CHECK(cr::times(v, 2) == cc::vector{4, 3, 1, 2, 4, 3, 1, 2});
    CHECK(cr::times(v, 3) == cc::vector{4, 3, 1, 2, 4, 3, 1, 2, 4, 3, 1, 2});
    CHECK(cr::times(cc::vector<int>{}, 0) == cc::vector<int>{});
    CHECK(cr::times(cc::vector<int>{}, 1) == cc::vector<int>{});
    CHECK(cr::times(cc::vector<int>{}, 2) == cc::vector<int>{});

    // reminder: int v[] = {4, 3, 1, 2};
    //           v_uneq0 = {4, 5, 1, 2};
}

TEST("cr modifying algorithms")
{
    cc::vector<int> v;
    cc::array<int> v2;
    cc::array<int> v3;
    int x = 10;

    struct foo
    {
        int v;

        foo() = default;
        explicit foo(int v) : v(v) {}
    };
    cc::vector<foo> fv;

    v = {2, 3, 4};
    cr::single(v, is_odd) = 10;
    CHECK(v == cc::vector{2, 10, 4});

    v = {2, 3, 4};
    cr::first(v) = 10;
    CHECK(v == cc::vector{10, 3, 4});

    v = {2, 3, 4};
    cr::first(v, is_odd) = 10;
    CHECK(v == cc::vector{2, 10, 4});

    v = {2, 3, 4};
    cr::min(v) = 10;
    CHECK(v == cc::vector{10, 3, 4});

    // cr::min(v, plus_one) = 3; // ERROR: expression is not assignable (this is good)

    v = {2, 3, 4};
    cr::max(v) = 10;
    CHECK(v == cc::vector{2, 3, 10});

    v = {4, 2, 3};
    {
        auto&& [vmin, vmax] = cr::minmax(v);
        vmin = 7;
        vmax = 9;
    }
    CHECK(v == cc::vector{9, 7, 3});

    v = {4, 2, 3};
    cr::min_by(v, mod3) = 10;
    CHECK(v == cc::vector{4, 2, 10});

    v = {4, 2, 3};
    cr::max_by(v, mod3) = 10;
    CHECK(v == cc::vector{4, 10, 3});

    v = {4, 2, 3};
    {
        auto&& [vmin, vmax] = cr::minmax_by(v, mod3);
        vmin = 7;
        vmax = 9;
    }
    CHECK(v == cc::vector{4, 9, 7});

    v = {1, 2, 3};
    cr::fill(v, 17);
    CHECK(v == cc::vector{17, 17, 17});

    v = {1, 2, 3};
    v2 = {7, 8};
    cr::copy_from(v, v2);
    CHECK(v == cc::vector{7, 8, 3});

    v = {1, 2, 3};
    v2 = {7, 8};
    cr::move_from(v, v2);
    CHECK(v == cc::vector{7, 8, 3});

    v = {4, 2, 1, 3};
    cr::find(v, is_odd) = 10;
    CHECK(v == cc::vector{4, 2, 10, 3});

    v = {4, 2, 1, 3};
    cr::find_last(v, is_odd) = 10;
    CHECK(v == cc::vector{4, 2, 1, 10});

    v = {4, 2, 1, 3};
    x = 9;
    cr::find_or(v, is_odd, x) = 10;
    CHECK(v == cc::vector{4, 2, 10, 3});
    CHECK(x == 9);

    v = {4, 2, 1, 3};
    x = 9;
    cr::find_or(v, is_negative, x) = 10;
    CHECK(v == cc::vector{4, 2, 1, 3});
    CHECK(x == 10);

    v = {4, 2, 1, 3};
    x = 9;
    cr::find_last_or(v, is_odd, x) = 10;
    CHECK(v == cc::vector{4, 2, 1, 10});
    CHECK(x == 9);

    v = {4, 2, 1, 3};
    x = 9;
    cr::find_last_or(v, is_negative, x) = 10;
    CHECK(v == cc::vector{4, 2, 1, 3});
    CHECK(x == 10);

    v = {3, 2, 6};
    cr::element_at(v, 1) = 9;
    CHECK(v == cc::vector{3, 9, 6});

    v = {3, 2, 6};
    x = 9;
    cr::element_at_or(v, 0, x) = 10;
    CHECK(v == cc::vector{10, 2, 6});
    CHECK(x == 9);

    v = {3, 2, 6};
    x = 9;
    cr::element_at_or(v, 3, x) = 10;
    CHECK(v == cc::vector{3, 2, 6});
    CHECK(x == 10);

    v = {1, 2, 3};
    x = 10;
    cr::for_each(v, [&](auto& i) {
        i += 7;
        ++x;
    });
    CHECK(v == cc::vector{8, 9, 10});
    CHECK(x == 13);

    v = {1, 2, 3};
    cr::where(v, is_odd).fill(4);
    CHECK(v == cc::vector{4, 2, 4});

    v = {4, 2, 1};
    v2 = {3, 2, 1, 0};
    v3 = {1, 2, 3, 4, 5};
    for (auto&& [a, b, c] : cr::zip(v, v2, v3))
    {
        c += a - b;

        if (a == b)
        {
            a += 1;
            b -= 1;
        }
    }
    CHECK(v == cc::vector{4, 3, 2});
    CHECK(cr::to<cc::vector>(v2) == cc::vector{3, 1, 0, 0});
    CHECK(cr::to<cc::vector>(v3) == cc::vector{2, 2, 3, 4, 5});

    v = {3, 2, 5};
    cr::each(v) += 3;
    CHECK(v == cc::vector{6, 5, 8});

    v = {2, 1, 3};
    x = 7;
    cr::pad_with(v, x).take(6).each() += 2;
    CHECK(v == cc::vector{4, 3, 5});
    CHECK(x == 13);

    v = {2, 1, 3};
    for (auto&& [idx, val] : cr::indexed(v))
        val += int(idx);
    CHECK(v == cc::vector{2, 2, 5});

    fv = {foo(2), foo(5)};
    for (auto&& [idx, val] : cr::indexed(fv))
        val.v += int(idx) + 1;
    CHECK(cr::map(fv, &foo::v) == cc::vector{3, 7});

    fv = {foo(2), foo(5)};
    for (auto&& [idx, val] : cr::indexed(fv, &foo::v))
        val += int(idx) + 1;
    CHECK(cr::map(fv, &foo::v) == cc::vector{3, 7});
}
