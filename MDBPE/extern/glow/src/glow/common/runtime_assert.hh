#pragma once

#include <typed-geometry/feature/assert.hh>

#include "log.hh"

/// Special macro for checking runtime invariants
/// Executes REACTION parameter on error
#define GLOW_RUNTIME_ASSERT(CONDITION, MSG, REACTION)      \
    do                                                     \
    {                                                      \
        if (!(CONDITION))                                  \
        {                                                  \
            glow::error() << "Runtime violation: " << MSG; \
            TG_ASSERT(false);                              \
            REACTION;                                      \
        }                                                  \
    } while (0)
