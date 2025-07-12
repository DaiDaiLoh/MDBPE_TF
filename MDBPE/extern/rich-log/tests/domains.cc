#include <nexus/test.hh>

#include <rich-log/log.hh>
#include <rich-log/logger.hh>

RICH_LOG_DECLARE_DOMAIN(Test);

RICH_LOG_DECLARE_DOMAIN(Other);
RICH_LOG_DEFINE_DOMAIN(Other, "other");

TEST("domains")
{
    auto has_default = false;
    auto has_test = false;
    auto has_other = false;

    for (auto d : rlog::get_domains())
    {
        auto name = cc::string_view(d->name);

        if (name == "default")
            has_default = true;
        else if (name == "test")
            has_test = true;
        else if (name == "other")
            has_other = true;
    }

    CHECK(has_default);
    CHECK(has_other);
    CHECK(has_test);
}

namespace mylib
{
RICH_LOG_DECLARE_DEFAULT_DOMAIN();
RICH_LOG_DECLARE_DOMAIN(Custom);

RICH_LOG_DEFINE_DEFAULT_DOMAIN("mylib.default");
RICH_LOG_DEFINE_DOMAIN(Custom, "mylib.custom");

TEST("domains in namespaces")
{
    cc::string domain_name;
    auto _ = rlog::scoped_logger_override(
        [&](rlog::message_ref m, bool&)
        {
            domain_name = m.domain->name;
            return true;
        });

    CHECK(domain_name == "");

    LOG("hello");
    CHECK(domain_name == "mylib.default");

    LOGD(Default, Info, "hello");
    CHECK(domain_name == "mylib.default");

    LOGD(Custom, Info, "hello");
    CHECK(domain_name == "mylib.custom");

    LOG_WARN("hello");
    CHECK(domain_name == "mylib.default");
}
}
