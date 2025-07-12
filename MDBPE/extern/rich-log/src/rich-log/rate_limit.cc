#include "rate_limit.hh"

#include <chrono>

bool rlog::rate::once::try_log()
{
    if (was_logged)
        return false;

    was_logged = true;
    return true;
}

bool rlog::rate::time_limited::try_log()
{
    auto t = std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    if (last_fired + interval_sec > t)
        return false;

    last_fired = t;
    return true;
}
