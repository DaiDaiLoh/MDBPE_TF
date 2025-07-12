#include <nexus/test.hh>

#include <clean-ranges/algorithms/count.hh>
#include <clean-ranges/algorithms/element_at.hh>
#include <clean-ranges/algorithms/find.hh>
#include <clean-ranges/algorithms/for_each.hh>
#include <clean-ranges/algorithms/single.hh>
#include <clean-ranges/algorithms/sum.hh>
#include <clean-ranges/range.hh>

#include <clean-core/unique_ptr.hh>
#include <clean-core/vector.hh>

TEST("range basics")
{
    cc::vector<int> v = {1, 2, 3, 4, 5};
    int a[] = {3, 10, 7};

    {
        auto r = cr::range(v);
        static_assert(r.is_view);
        static_assert(!r.is_owning);
        static_assert(!r.is_readonly);
    }

    {
        auto r = cr::range(cc::vector(v));
        static_assert(!r.is_view);
        static_assert(r.is_owning);
        static_assert(!r.is_readonly);
    }

    {
        auto r = cr::range(cc::vector<int>{1, 2, 3});
        static_assert(!r.is_view);
        static_assert(r.is_owning);
        static_assert(!r.is_readonly);
    }

    {
        auto const& cv = v;
        auto r = cr::range(cv);
        static_assert(r.is_view);
        static_assert(!r.is_owning);
        static_assert(r.is_readonly);
    }

    static_assert(cr::range({5, 3, 2}).sum() == 10);

    CHECK(cr::range(v).sum() == 15);
    CHECK(cr::range(a).sum() == 20);

    cr::range(v).container()[1] += 10;
    CHECK(cr::range(v).sum() == 25);

    for (auto& i : cr::range(v))
        --i;

    CHECK(cr::range(v).sum() == 20);
    CHECK(cr::range(v.begin(), v.end()).sum() == 20);

    CHECK(cr::range(10).sum() == 10 * 9 / 2);
    CHECK(cr::range(5, 8).sum() == 5 + 6 + 7);
    CHECK(cr::range(5, 11, 2).sum() == 5 + 7 + 9);

    CHECK(cr::inf_range(0).element_at(100) == 100);
}

TEST("range general call")
{
    struct foo
    {
        int x;
        int f() const { return x + 1; }
        bool is_two() const { return x == 2; }

        void dec() { --x; }

        foo() = default;
        explicit foo(int v) : x(v) {}
    };

    auto f = [](foo const& f) { return f.x * f.x; };

    cc::vector<foo> v;

    v.push_back(foo{1});
    v.push_back(foo{2});
    v.push_back(foo{3});

    cc::vector<foo*> pv;
    pv.push_back(&v[0]);
    pv.push_back(&v[2]);

    cc::vector<cc::unique_ptr<foo>> sv;
    sv.emplace_back(cc::make_unique<foo>(7));
    sv.emplace_back(cc::make_unique<foo>(3));
    sv.emplace_back(cc::make_unique<foo>(1));

    CHECK(cr::sum(v, f) == 1 + 4 + 9);

    CHECK(cr::sum(v, &foo::x) == 6);
    CHECK(cr::sum(v, &foo::f) == 9);

    CHECK(cr::sum(pv, &foo::x) == 4);
    CHECK(cr::sum(pv, &foo::f) == 6);

    CHECK(cr::sum(sv, &foo::x) == 11);
    CHECK(cr::sum(sv, &foo::f) == 14);

    CHECK(cr::single(v, &foo::is_two).x == 2);
    cr::single(v, &foo::is_two).x = 10;
    CHECK(cr::sum(v, &foo::x) == 1 + 10 + 3);

    cr::for_each(v, &foo::dec);
    CHECK(cr::sum(v, &foo::x) == 0 + 9 + 2);
}

TEST("range general call tricky")
{
    struct tricky
    {
        int x;
        int value() const { return x; }

        tricky& operator*() { return *this; }
    };
    cc::vector<tricky> v;
    v.push_back({3});
    v.push_back({5});
    v.push_back({7});

    CHECK(cr::sum(v, &tricky::x) == 15);
    CHECK(cr::sum(v, &tricky::value) == 15);
}

TEST("range index support")
{
    {
        cc::vector<int> v = {4, 2, 3};
        CHECK(cr::sum(v, [](int idx, int v) { return idx * v; }) == 0 * 4 + 1 * 2 + 2 * 3);
    }

    {
        struct foo
        {
            int x;
            int f() const { return x + 1; }

            bool is_above_idx(int idx) const { return x > idx; }
        };

        cc::vector<foo> v;

        v.push_back(foo{5});
        v.push_back(foo{1});
        v.push_back(foo{3});

        CHECK(cr::find(v, [](int idx, foo const& f) { return idx == f.x; }).x == 1);
        CHECK(cr::last(v, &foo::is_above_idx).x == 3);
        CHECK(cr::count_if(v, &foo::is_above_idx) == 2);
    }
}
