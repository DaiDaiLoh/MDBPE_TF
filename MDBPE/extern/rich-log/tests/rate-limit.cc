#include <nexus/test.hh>

#include <rich-log/log.hh>
#include <rich-log/logger.hh>

RICH_LOG_DECLARE_DOMAIN(Test);

TEST("logger cooldown verbosity")
{
    int msg_cnt = 0;
    auto _ = rlog::scoped_logger_override(
        [&](rlog::message_ref, bool&)
        {
            msg_cnt++;
            return true;
        });

    LOG("capture me");
    CHECK(msg_cnt == 1);

    for (auto i = 0; i < 10; ++i)
        LOG("many captures");
    CHECK(msg_cnt == 11);

    rlog::rate::once _once;
    for (auto i = 0; i < 10; ++i)
        LOGD_ONCE(_once, Default, Info, "many logs, captured once");
    CHECK(msg_cnt == 12);
}
