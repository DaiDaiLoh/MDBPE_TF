#pragma once

#include <clean-core/macros.hh>

#ifdef CC_OS_WINDOWS

#ifdef RLOG_BUILD_DLL

#ifdef RLOG_DLL
#define RLOG_API __declspec(dllexport)
#else
#define RLOG_API __declspec(dllimport)
#endif

#else
#define RLOG_API
#endif

#else
#define RLOG_API
#endif
