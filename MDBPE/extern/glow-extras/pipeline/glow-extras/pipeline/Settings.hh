#pragma once

// ---- CMake-option based settings ----

#if defined GLOW_EXTRAS_HAS_AION && defined GLOW_EXTRAS_PIPELINE_ENABLE_CPU_PROFILING
#include <aion/Tracer.hh>
#define PIPELINE_PROFILE(_name_) TRACE(_name_)
#define PIPELINE_PROFILE_OUTPUT() aion::tracing::write_speedscope_json("pipeline_speedscope.json")
#else
#define PIPELINE_PROFILE(_name_) (void)0
#define PIPELINE_PROFILE_OUTPUT() (void)0
#endif
