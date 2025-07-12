#pragma once

#include <limits>

namespace rlog::rate
{
// once, once_per_sec etc
// usable per invoc, static etc

struct log_rate_limiter
{
    /// returns true if logging should succeed (and might change internal state)
    virtual bool try_log() = 0;
    virtual ~log_rate_limiter() = default;
};

/// example usage:
///
///   // will warn once globally
///   static rlog::rate::once _once;
///   LOG_WARN_ONCE(_once, "something happened ...", ...);
///
///   // will warn once per invocation
///   void foo()
///   {
///       rlog::rate::once _once;
///       for (...)
///       {
///           ...
///           LOG_WARN_ONCE(_once, "something happened ...", ...);
///       }
///   }
///
struct once : log_rate_limiter
{
    bool was_logged = false;

    bool try_log() override;
};

struct time_limited : log_rate_limiter
{
    double interval_sec = 1;
    double last_fired = std::numeric_limits<double>::lowest();

    bool try_log() override;
};
}
