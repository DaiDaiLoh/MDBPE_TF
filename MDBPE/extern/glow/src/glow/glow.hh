#pragma once

namespace glow
{
/// OpenGL Version information
/// Only valid after initGLOW
extern struct _glowGLVersion
{
    int major;
    int minor;
    int total; // like 43 for 4.3
} OGLVersion;

/**
 * @brief initializes the GLOW Library
 *
 * In particular:
 *   - loads OpenGL function
 *   - performs certain static initialization
 *
 * @return true on success
 */
bool initGLOW();

/// returns true if glow is initialized in this thread
bool isInitialized();

/// Asserts that glow is initialized
/// Fails if not initialized OR wrong thread
#ifdef GLOW_PERFORM_VALIDATION
void checkValidGLOW();
#else
#define checkValidGLOW() (void)0
#endif
}
