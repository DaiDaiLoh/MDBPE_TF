#pragma once

#ifdef GLOW_THREAD_AGNOSTIC

#define GLOW_THREADLOCAL

#else

#ifdef _MSC_VER
#define GLOW_THREADLOCAL __declspec(thread)
#else
#define GLOW_THREADLOCAL __thread // GCC 4.7 has no thread_local yet
#endif

#endif
