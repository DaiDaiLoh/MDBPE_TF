#include <cstdint>

#include <nexus/test.hh>

#include <clean-core/array.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/file.hh>

TEST("file")
{
    auto tmp_file = "_tmp_babel_file";
    babel::file::write(tmp_file, "hello world!");
    CHECK(babel::file::read_all_text(tmp_file) == "hello world!");

    babel::file::write_lines(tmp_file, cc::make_range_ref({"hello", "world"}));
    CHECK(babel::file::read_all_text(tmp_file) == "hello\nworld");

    babel::file::write(tmp_file, cc::as_byte_span(cc::vector<uint8_t>{100, 200, 50}));
    auto d = babel::file::read_all_bytes(tmp_file);
    CHECK(d == cc::array<std::byte>{std::byte(100), std::byte(200), std::byte(50)});
}
