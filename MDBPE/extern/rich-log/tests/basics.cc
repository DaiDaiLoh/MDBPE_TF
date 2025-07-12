#include <nexus/test.hh>

#include <rich-log/log.hh>
#include <rich-log/logger.hh>

RICH_LOG_DECLARE_DOMAIN(Test);
RICH_LOG_DEFINE_DOMAIN(Test, "test");

TEST("basic logging", disabled) // call directly to run this test (it will print to console)
{
    rlog::enable_win32_colors();
    rlog::set_current_thread_name("td#0");

    LOG("hello world!");
    LOG("{1} {0}!", "world", "hello");

    LOGD(Test, Warning, "'tis a warning, mate");
    LOGD(Test, Info, "information follows at %s", 12345);

    LOG("multi\nline\nlog message");

    LOGD(Test, Error, "an error occured"); // debugger should break here

    CHECK(true); // disable warning
}
