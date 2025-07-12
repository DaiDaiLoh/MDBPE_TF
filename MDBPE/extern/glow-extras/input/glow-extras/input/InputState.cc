#include "InputState.hh"

void glow::input::InputState::reset()
{
    mSnapshotCurrent = InputSnapshot{};
    mSnapshotPrevious = InputSnapshot{};
}
