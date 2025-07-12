#include <nexus/fuzz_test.hh>

#include <babel-serializer/compression/zstd.hh>

FUZZ_TEST("zstd fuzzer")(tg::rng& rng)
{
    auto cnt = uniform(rng, 0, 10);
    if (uniform(rng))
        cnt = uniform(rng, 100, 1000);

    auto orig_data = cc::vector<std::byte>(cnt);
    for (auto& d : orig_data)
        d = uniform(rng);

    auto comp_data = babel::zstd::compress(orig_data);

    auto uncomp_data = babel::zstd::uncompress(comp_data);

    CHECK(orig_data == uncomp_data);
}
