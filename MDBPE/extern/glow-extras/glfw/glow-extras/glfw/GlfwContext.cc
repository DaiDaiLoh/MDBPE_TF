#include "GlfwContext.hh"

#include <cstring>

#include <typed-geometry/feature/assert.hh>

#include <glow/common/log.hh>
#include <glow/common/thread_local.hh>
#include <glow/gl.hh>
#include <glow/glow.hh>
#include <glow/util/AsyncTextureLoader.hh>

#include <GLFW/glfw3.h>

using namespace glow;
using namespace glfw;

namespace
{
GLOW_THREADLOCAL GlfwContext* gContext = nullptr;
}

GlfwContext::GlfwContext()
{
    TG_ASSERT(!gContext);
    if (gContext)
    {
        std::cerr << "A GlfwContext already exists and cannot be created twice" << std::endl;
        return;
    }

    auto title = "GLFW Window";

    // Taken from http://www.glfw.org/documentation.html
    // Initialize the library
    if (!glfwInit())
    {
        std::cerr << "Unable to initialize GLFW" << std::endl;
        return;
    }

    // invisible window
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // Request debug context
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    // Try to get core context
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GLOW_OPENGL_VERSION / 10);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GLOW_OPENGL_VERSION % 10);

    // Create a windowed mode window and its OpenGL context
    mWindow = glfwCreateWindow(1, 1, title, nullptr, nullptr);
    if (!mWindow)
    {
        std::cerr << "Unable to get OpenGL " << GLOW_OPENGL_VERSION / 10 << "." << GLOW_OPENGL_VERSION % 10
                  << " Core Debug Context. Trying again with Compat." << std::endl;
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_FALSE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        mWindow = glfwCreateWindow(1, 1, "GLFW Window", nullptr, nullptr);

        if (!mWindow)
        {
            std::cerr << "Unable to create a GLFW window" << std::endl;
            glfwTerminate();
            return;
        }
    }

    // Make the window's context current
    glfwMakeContextCurrent(mWindow);

    // WORKAROUND for Intel bug (3.3 available but 3.0 returned UNLESS explicitly requested)
#if defined GLOW_COMPILER_MSVC && _WIN32
#define GL_CALL __stdcall // 32bit windows needs a special calling convention
#else
#define GL_CALL
#endif

    using glGetStringFunc = GLubyte* GL_CALL(GLenum);
    auto getString = reinterpret_cast<glGetStringFunc*>(glfwGetProcAddress("glGetString"));

    // taken from glad
    GLint gmajor;
    GLint gminor;

    const char* version;
    const char* prefixes[] = {"OpenGL ES-CM ", "OpenGL ES-CL ", "OpenGL ES ", NULL};

    version = (const char*)getString(GL_VERSION);
    if (!version)
        return;

    for (auto i = 0; prefixes[i]; i++)
    {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0)
        {
            version += length;
            break;
        }
    }

#ifdef _MSC_VER
    sscanf_s(version, "%d.%d", &gmajor, &gminor);
#else
    sscanf(version, "%d.%d", &gmajor, &gminor);
#endif

    if (gmajor * 10 + gminor < 33)
    {
        std::cerr << "OpenGL Version below 3.3. Trying to get at least 3.3." << std::endl;

        // destroy current window
        glfwDestroyWindow(mWindow);

        // request vanilla 3.3 context
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_FALSE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#endif

        // re-try window creation
        mWindow = glfwCreateWindow(1, 1, title, nullptr, nullptr);
        if (!mWindow)
        {
            std::cerr << "Unable to create a GLFW window with OpenGL 3.3. (GLOW requires at least 3.3)" << std::endl;
            glfwTerminate();
            return;
        }

        // Make the window's context current (again)
        glfwMakeContextCurrent(mWindow);
    }

    // Initialize GLOW
    if (!glow::initGLOW())
    {
        std::cerr << "Unable to initialize GLOW" << std::endl;
        return;
    }

    // restore ogl state
    glow::restoreDefaultOpenGLState();

    gContext = this;
}

GlfwContext::~GlfwContext()
{
    TG_ASSERT(gContext == this);
    gContext = nullptr;

    glfwTerminate();
    AsyncTextureLoader::shutdown();
    mWindow = nullptr;
}

GlfwContext* GlfwContext::current() { return gContext; }

void GlfwContext::setTitle(const std::string& title)
{
    TG_ASSERT(isValid());
    glfwSetWindowTitle(mWindow, title.c_str());
}

void GlfwContext::resize(int w, int h)
{
    TG_ASSERT(isValid());
    if (mIsFullscreen)
        leaveFullscreen(w, h, -1, -1);
    else
        glfwSetWindowSize(mWindow, w, h);
}

void GlfwContext::reposition(int x, int y)
{
    TG_ASSERT(isValid());
    if (mIsFullscreen)
        leaveFullscreen(-1, -1, x, y);
    else
        glfwSetWindowPos(mWindow, x, y);
}

void GlfwContext::show(int w, int h, int x, int y)
{
    TG_ASSERT(isValid());
    if (w > 0 && h > 0)
        resize(w, h);
    if (x >= 0 && y >= 0)
        reposition(x, y);
    glfwShowWindow(mWindow);
}

void GlfwContext::hide()
{
    TG_ASSERT(isValid());
    glfwHideWindow(mWindow);
}

void GlfwContext::swapBuffers()
{
    TG_ASSERT(isValid());
    glfwSwapBuffers(mWindow);
}

void GlfwContext::leaveFullscreen(int w, int h, int x, int y)
{
    TG_ASSERT(isValid());

    if (!mIsFullscreen)
        return;

    if (x == -1)
        x = mCachedPositionX;
    if (y == -1)
        y = mCachedPositionY;
    if (w == -1)
        w = mCachedWidth;
    if (h == -1)
        h = mCachedHeight;

    auto const* videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowMonitor(mWindow, nullptr, x, y, w, h, videoMode->refreshRate);
    mIsFullscreen = false;
}

void GlfwContext::enterFullscreen(GLFWmonitor* monitor)
{
    TG_ASSERT(isValid());

    if (!monitor)
        monitor = glfwGetPrimaryMonitor();

    // Cache current window position and size
    glfwGetWindowSize(mWindow, &mCachedWidth, &mCachedHeight);
    glfwGetWindowPos(mWindow, &mCachedPositionX, &mCachedPositionY);

    auto const* videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowMonitor(mWindow, glfwGetPrimaryMonitor(), 0, 0, videoMode->width, videoMode->height, videoMode->refreshRate);
    mIsFullscreen = true;
}

void GlfwContext::toggleFullscreen()
{
    if (mIsFullscreen)
        leaveFullscreen();
    else
        enterFullscreen();
}

void GlfwContext::setCachedSize(int w, int h)
{
    mCachedWidth = w;
    mCachedHeight = h;
}

void GlfwContext::setCachedPosition(int x, int y)
{
    mCachedPositionX = x;
    mCachedPositionY = y;
}
