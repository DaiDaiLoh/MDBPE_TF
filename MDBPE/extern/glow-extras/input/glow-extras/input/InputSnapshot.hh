#pragma once

#include <algorithm>
#include <vector>

#include <typed-geometry/tg-lean.hh>

namespace glow
{
namespace input
{
struct InputSnapshot
{
    std::vector<int> pressedKeys;
    std::vector<int> pressedMouseButtons;

    tg::dpos2 mousePosition = {0, 0};
    tg::pos2 scrollPosition = {0, 0};

public:
    // -- Manipulators --
    void addKey(int key) { pressedKeys.push_back(key); }
    void removeKey(int key) { pressedKeys.erase(std::remove(pressedKeys.begin(), pressedKeys.end(), key), pressedKeys.end()); }

    void addMouseButton(int btn) { pressedMouseButtons.push_back(btn); }
    void removeMouseButton(int btn)
    {
        pressedMouseButtons.erase(std::remove(pressedMouseButtons.begin(), pressedMouseButtons.end(), btn), pressedMouseButtons.end());
    }

    void addMouseDelta(tg::dvec2 const& delta);
    void addScrollDelta(tg::vec2 const& delta);

public:
    // -- Getters --
    bool isKeyDown(int key) const { return std::find(pressedKeys.begin(), pressedKeys.end(), key) != pressedKeys.end(); }
    bool isKeyUp(int key) const { return !isKeyDown(key); }
    bool isKeyPressed(InputSnapshot const& prev, int key) const { return isKeyDown(key) && prev.isKeyUp(key); }
    bool isKeyReleased(InputSnapshot const& prev, int key) const { return isKeyUp(key) && prev.isKeyDown(key); }

    bool isMouseButtonDown(int btn) const
    {
        return std::find(pressedMouseButtons.begin(), pressedMouseButtons.end(), btn) != pressedMouseButtons.end();
    }
    bool isMouseButtonUp(int btn) const { return !isMouseButtonDown(btn); }
    bool isMouseButtonPressed(InputSnapshot const& prev, int btn) const { return isMouseButtonDown(btn) && prev.isMouseButtonUp(btn); }
    bool isMouseButtonReleased(InputSnapshot const& prev, int btn) const { return isMouseButtonUp(btn) && prev.isMouseButtonDown(btn); }

    tg::dvec2 getMouseDelta(InputSnapshot const& prev) const;
    tg::vec2 getScrollDelta(InputSnapshot const& prev) const;
};
}
}
