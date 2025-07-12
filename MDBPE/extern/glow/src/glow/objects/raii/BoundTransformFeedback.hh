#pragma once

#include <vector>

#include <glow/common/non_copyable.hh>
#include <glow/fwd.hh>
#include <glow/gl.hh>

namespace glow
{
/// RAII-object that defines a "bind"-scope for an TransformFeedback
/// All functions that operate on the currently bound object are accessed here
struct BoundTransformFeedback
{
    GLOW_RAII_CLASS(BoundTransformFeedback);

    /// Backreference to the feedback
    TransformFeedback* const feedback;

    /// Sets and binds a new feedback buffer (not valid while recording)
    void setFeedbackBuffer(SharedBuffer const& feedbackBuffer);

    /// Starts the transform feedback (no nesting allowed!)
    void begin(GLenum primitiveMode);
    /// Ends the transform feedback (no-op if not recording)
    void end();

private:
    GLint previousFeedback;                      ///< previously bound feedback
    BoundTransformFeedback* previousFeedbackPtr; ///< previously bound feedback
    BoundTransformFeedback(TransformFeedback* feedback);
    friend class TransformFeedback;

    /// returns true iff it's safe to use this bound class
    /// otherwise, runtime error
    bool isCurrent() const;

public:
    BoundTransformFeedback(BoundTransformFeedback&&); // allow move
    ~BoundTransformFeedback();
};
}
