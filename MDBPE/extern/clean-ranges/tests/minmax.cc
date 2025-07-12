#include <nexus/test.hh>

#include <clean-core/vector.hh>

#include <clean-ranges/algorithms/minmax.hh>

#include <typed-geometry/tg.hh>

TEST("cr::min/max with tg")
{
    cc::vector<tg::pos3> v;
    auto bb = tg::aabb3(-10, 10);
    tg::rng rng;
    for (auto i = 0; i < 10; ++i)
        v.push_back(uniform(rng, bb));

    auto pmin = cr::min(v);
    auto pmax = cr::max(v);

    for (auto const& p : v)
    {
        CHECK(pmin.x <= p.x);
        CHECK(pmin.y <= p.y);
        CHECK(pmin.z <= p.z);

        CHECK(p.x <= pmax.x);
        CHECK(p.y <= pmax.y);
        CHECK(p.z <= pmax.z);
    }
}
