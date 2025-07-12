#include <nexus/test.hh>

#include <babel-serializer/source_map.hh>

TEST("source_map")
{
    {
        cc::string_view s = "hello\nworld";
        auto map = babel::source_map(s);
        CHECK(map.lines().size() == 2);

        CHECK(map.line_of(&s[0]) == 0);
        CHECK(map.line_of(&s[4]) == 0);
        CHECK(map.line_of(&s[5]) == 0);
        CHECK(map.line_of(&s[6]) == 1);
        CHECK(map.line_of(&s[10]) == 1);
    }
    {
        cc::string_view s = "\n\n\n\n";
        auto map = babel::source_map(s);
        CHECK(map.lines().size() == 4);

        CHECK(map.line_of(&s[0]) == 0);
        CHECK(map.line_of(&s[1]) == 1);
        CHECK(map.line_of(&s[2]) == 2);
        CHECK(map.line_of(&s[3]) == 3);
    }
    {
        cc::string_view s = "";
        auto map = babel::source_map(s);
        CHECK(map.lines().size() == 0);
    }
}
