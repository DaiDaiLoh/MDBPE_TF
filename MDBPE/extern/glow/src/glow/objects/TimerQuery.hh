#pragma once

#include "Query.hh"

namespace glow
{
GLOW_SHARED(class, TimerQuery);
/**
 * TimerQueries can measure GPU execution speed.
 *
 * Only available since OpenGL 3.3 or GL_ARB_timer_query (on OpenGL 3.2)
 */
class TimerQuery final : public Query
{
public:
    TimerQuery() : Query(GL_TIME_ELAPSED) {}

    /// Converts a ns time value to seconds
    static double toSeconds(GLuint64 timeInNs) { return timeInNs / (1000. * 1000. * 1000.); }

    float elapsedSeconds() { return float(toSeconds(getResult64())); }
    double elapsedSecondsD() { return toSeconds(getResult64()); }

public:
    static SharedTimerQuery create() { return std::make_shared<TimerQuery>(); }
};
}
