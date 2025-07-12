#pragma once

#include <glow/common/nodiscard.hh>
#include <glow/common/non_copyable.hh>
#include <glow/common/shared.hh>

#include <glow/gl.hh>

#include "NamedObject.hh"

#include "raii/BoundTransformFeedback.hh"

namespace glow
{
GLOW_SHARED(class, Buffer);
GLOW_SHARED(class, TransformFeedback);

/**
 * @brief A TransformFeedback object encapsulates and manages state for Transform Feedback
 *
 * This includes:
 *   - The actual feedbacks used for feedback
 *   - Recorded primitive count
 *
 * Usage:
 *   - init:
 *       program->configureTransformFeedback(...)
 *       feedback = TransformFeedback::create(buffer);
 *   - rendering:
 *       auto shader = program->use();
 *       myVAO->bind().negotiateBindings(); // important bcz relinking cannot happen during transform feedback
 *       {
 *         auto fb = feedback->bind();
 *         fb->begin();
 *         ... render myVAO
 *         fb->end();
 *       }
 *       otherVAO->bind().drawTransformFeedback(feedback);
 */
class TransformFeedback final : public NamedObject<TransformFeedback, GL_TRANSFORM_FEEDBACK>
{
private:
    /// OGL id
    GLuint mObjectName;

    /// True iff currently recording
    bool mIsRecording = false;

    /// Buffer bound to GL_TRANSFORM_FEEDBACK_BUFFER
    /// Can only be set while bount (and not recording)
    SharedBuffer mFeedbackBuffer;

public: // getter
    /// Gets the currently bound TransformFeedback (nullptr if none)
    static BoundTransformFeedback* getCurrentFeedback();

    GLuint getObjectName() const { return mObjectName; }
    bool isRecording() const { return mIsRecording; }
    bool isBound() const { return getCurrentFeedback() != nullptr && getCurrentFeedback()->feedback == this; }
    SharedBuffer const& getFeedbackBuffer() const { return mFeedbackBuffer; }

public:
    /// Creates a new TransformFeedback object
    TransformFeedback();
    ~TransformFeedback();

    /// Binds this transform feedback.
    /// Unbinding is done when the returned object runs out of scope.
    GLOW_NODISCARD BoundTransformFeedback bind() { return {this}; }
    friend BoundTransformFeedback;

public:
    /// Creates a new TransformFeedback
    static SharedTransformFeedback create(SharedBuffer const& feedbackBuffer = nullptr);
};
}
