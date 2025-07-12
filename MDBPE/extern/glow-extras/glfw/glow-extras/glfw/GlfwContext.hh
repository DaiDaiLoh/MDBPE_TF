#pragma once

#include <string>

struct GLFWwindow;
struct GLFWmonitor;

namespace glow
{
namespace glfw
{
/**
 * Creates an invisible Glfw window that provides an OpenGL context
 *
 * Also registers itself as a threadlocal global variable so that it can be accessed from everywhere
 *
 * Note: CANNOT be nested currently
 */
class GlfwContext final
{
public:
    GlfwContext();
    ~GlfwContext();

    GLFWwindow* window() const { return mWindow; }

    bool isValid() const { return mWindow != nullptr; }
    bool isFullscreen() const { return mIsFullscreen; }

    static GlfwContext* current();

    void setTitle(std::string const& title);

    void resize(int w, int h);
    void reposition(int x, int y);
    void show(int w = -1, int h = -1, int x = -1, int y = -1);
    void hide();
    void swapBuffers();

    void leaveFullscreen(int w = -1, int h = -1, int x = -1, int y = -1);
    void enterFullscreen(GLFWmonitor* monitor = nullptr);
    void toggleFullscreen();

    void setCachedSize(int w, int h);
    void setCachedPosition(int x, int y);

private:
    GLFWwindow* mWindow = nullptr;

    bool mIsFullscreen = false;
    int mCachedPositionX = -1;
    int mCachedPositionY = -1;
    int mCachedWidth = -1;
    int mCachedHeight = -1;
};

}
}
