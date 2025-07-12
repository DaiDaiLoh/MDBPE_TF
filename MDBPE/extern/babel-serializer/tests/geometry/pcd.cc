#include <nexus/test.hh>

#include <cstdint>

#include <clean-core/vector.hh>
#include <clean-ranges/algorithms/to.hh>

#include <babel-serializer/geometry/pcd.hh>

TEST("pcd ascii")
{
    cc::string_view file = R"(# .PCD v.7 - Point Cloud Data file format
VERSION .7
FIELDS x y z rgb
SIZE 4 4 4 4
TYPE F F F I
# comment line!
COUNT 1 1 1 3
WIDTH 5
HEIGHT 1
VIEWPOINT 0 0 0 1 0 0 0
POINTS 5
DATA ascii
0.93773 0.33763 0 1 2 3
0.90805 0.35641 0 4 5 6
0.81915 0.32 0 7 8 9
0.97192 0.278 0 10 11 12
0.944 0.29474 0 13 14 15)";

    auto pts = babel::pcd::read(cc::as_byte_span(file));
    CHECK(pts.fields.size() == 4);
    CHECK(pts.width == 5);
    CHECK(pts.height == 1);
    CHECK(pts.points == 5);

    auto cols = cr::to<cc::vector>(pts.get_data<tg::ivec3>("rgb"));
    CHECK(cols.size() == 5);
    CHECK(cols[0] == tg::ivec3(1, 2, 3));
    CHECK(cols[1] == tg::ivec3(4, 5, 6));
    CHECK(cols[2] == tg::ivec3(7, 8, 9));
    CHECK(cols[3] == tg::ivec3(10, 11, 12));
    CHECK(cols[4] == tg::ivec3(13, 14, 15));
}

TEST("pcd binary")
{
    cc::string_view file = R"(# .PCD v.7 - Point Cloud Data file format
VERSION .7
FIELDS a b
SIZE 1 1
TYPE U U
COUNT 1 1
WIDTH 3
HEIGHT 1
VIEWPOINT 0 0 0 1 0 0 0
POINTS 3
DATA binary
abcdef)";

    auto pts = babel::pcd::read(cc::as_byte_span(file));
    CHECK(pts.fields.size() == 2);
    CHECK(pts.width == 3);
    CHECK(pts.height == 1);
    CHECK(pts.points == 3);

    {
        auto cols = cr::to<cc::vector>(pts.get_data<uint8_t>("a"));
        CHECK(cols.size() == 3);
        CHECK(cols[0] == 'a');
        CHECK(cols[1] == 'c');
        CHECK(cols[2] == 'e');
    }
    {
        auto cols = cr::to<cc::vector>(pts.get_data<uint8_t>("b"));
        CHECK(cols.size() == 3);
        CHECK(cols[0] == 'b');
        CHECK(cols[1] == 'd');
        CHECK(cols[2] == 'f');
    }
}
