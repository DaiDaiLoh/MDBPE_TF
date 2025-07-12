#include "glow.hh"

#include <typed-geometry/feature/assert.hh>

#include "debug.hh"
#include "gl.hh"
#include "limits.hh"

#include "common/log.hh"
#include "common/thread_local.hh"

namespace
{
GLOW_THREADLOCAL bool _isGlowInitialized = false;
}

using namespace glow;

glow::_glowGLVersion glow::OGLVersion;

bool glow::initGLOW()
{
    if (_isGlowInitialized)
        return true;

    if (!gladLoadGL())
        return false;

    OGLVersion.major = GLVersion.major;
    OGLVersion.minor = GLVersion.minor;
    OGLVersion.total = OGLVersion.major * 10 + OGLVersion.minor;

    // install debug message
    auto dbgCallback = false;
    if (glDebugMessageCallback)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(DebugMessageCallback, nullptr);
        dbgCallback = true;
    }

    // BEFORE limits
    _isGlowInitialized = true;

    // update limits
    limits::update();

    // restore defaults
    restoreDefaultOpenGLState();

    // report context flags
    GLint flags, profile;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    std::string context = "Unknown";
    switch (profile)
    {
    case GL_CONTEXT_CORE_PROFILE_BIT:
        context = "Core";
        break;
    case GL_CONTEXT_COMPATIBILITY_PROFILE_BIT:
        context = "Compatibility";
        break;
    }
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        context += ", Debug";
        if (dbgCallback)
            context += " (with callback)";
    }
    if (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
        context += ", Forward Compatible";
    // if (flags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT)
    //     context += ", Robust Memory Access";
    // if (flags & GL_CONTEXT_FLAG_NO_ERROR_BIT)
    //     context += ", No-Error";

    auto const* gpuName = glGetString(GL_RENDERER);
    info() << "Loaded OpenGL " << OGLVersion.major << "." << OGLVersion.minor << " Context [" << context << "]"
           << " on " << gpuName;

    return true;
}

#ifdef GLOW_PERFORM_VALIDATION
void glow::checkValidGLOW()
{
    TG_ASSERT(_isGlowInitialized && "GLOW is not initialized OR called from wrong thread.");
    if (!_isGlowInitialized) // for release
        glow::error() << "ERROR: GLOW is not initialized OR called from wrong thread.";
}
#endif

bool glow::isInitialized() { return _isGlowInitialized; }
