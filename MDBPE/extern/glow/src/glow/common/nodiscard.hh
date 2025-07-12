#pragma once

#if __cplusplus >= 201703L
#define GLOW_NODISCARD [[nodiscard]]
#else
#ifdef GLOW_COMPILER_MSVC
#define GLOW_NODISCARD
#else
#define GLOW_NODISCARD __attribute__((warn_unused_result))
#endif
#endif
