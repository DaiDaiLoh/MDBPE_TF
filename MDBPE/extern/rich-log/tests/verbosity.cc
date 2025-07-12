#include <nexus/test.hh>

#include <rich-log/log.hh>
#include <rich-log/logger.hh>

RICH_LOG_DECLARE_DOMAIN_DETAIL(Test, Debug, extern);

TEST("domain min verbosity")
{
    cc::string msg;
    auto _ = rlog::scoped_logger_override(
        [&](rlog::message_ref m, bool&)
        {
            msg = m.message;
            return true;
        });

    LOG("capture me");
    CHECK(msg == "capture me");

    LOGD(Test, Debug, "me not logged");
    CHECK(msg == "capture me");

    Log::Test::domain.min_verbosity = rlog::verbosity::Debug;

    LOGD(Test, Debug, "now you see me");
    CHECK(msg == "now you see me");

    LOGD(Test, Trace, "still hidden");
    CHECK(msg == "now you see me");

    Log::Test::domain.min_verbosity = rlog::verbosity::Trace;

    LOGD(Test, Trace, "nope, this removed compile-time");
    CHECK(msg == "now you see me");
}
