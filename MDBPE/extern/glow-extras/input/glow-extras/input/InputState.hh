#pragma once

#include "InputSnapshot.hh"

namespace glow
{
namespace input
{
struct InputState
{
private:
    InputSnapshot mSnapshotCurrent;
    InputSnapshot mSnapshotPrevious;

public:
    // -- Events --
    void onKeyPress(int key) { mSnapshotCurrent.addKey(key); }
    void onKeyRelease(int key) { mSnapshotCurrent.removeKey(key); }

    void onMouseButtonPress(int button) { mSnapshotCurrent.addMouseButton(button); }
    void onMouseButtonRelease(int button) { mSnapshotCurrent.removeMouseButton(button); }

    void onMousePos(double x, double y) { mSnapshotCurrent.mousePosition = {x, y}; }
    void onScroll(float deltaX, float deltaY) { mSnapshotCurrent.addScrollDelta({deltaX, deltaY}); }

    /// Enter the next cycle (polling interval)
    void swap() { mSnapshotPrevious = mSnapshotCurrent; }

    /// Reset the input state
    void reset();

public:
    // -- Getters --
    bool isKeyDown(int key) const { return mSnapshotCurrent.isKeyDown(key); }
    bool isKeyUp(int key) const { return mSnapshotCurrent.isKeyUp(key); }
    bool isKeyPressed(int key) const { return mSnapshotCurrent.isKeyPressed(mSnapshotPrevious, key); }
    bool isKeyReleased(int key) const { return mSnapshotCurrent.isKeyReleased(mSnapshotPrevious, key); }

    bool isMouseButtonDown(int btn) const { return mSnapshotCurrent.isMouseButtonDown(btn); }
    bool isMouseButtonUp(int btn) const { return mSnapshotCurrent.isMouseButtonUp(btn); }
    bool isMouseButtonPressed(int btn) const { return mSnapshotCurrent.isMouseButtonPressed(mSnapshotPrevious, btn); }
    bool isMouseButtonReleased(int btn) const { return mSnapshotCurrent.isMouseButtonReleased(mSnapshotPrevious, btn); }

    tg::dpos2 const& getMousePosition() const { return mSnapshotCurrent.mousePosition; }
    tg::pos2 const& getScrollPosition() const { return mSnapshotCurrent.scrollPosition; }
    tg::dvec2 getMouseDelta() const { return mSnapshotCurrent.getMouseDelta(mSnapshotPrevious); }
    tg::vec2 getMouseDeltaF() const { return tg::vec2(mSnapshotCurrent.getMouseDelta(mSnapshotPrevious)); }
    tg::vec2 getScrollDelta() const { return mSnapshotCurrent.getScrollDelta(mSnapshotPrevious); }
};
}
}
