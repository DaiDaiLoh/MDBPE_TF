#pragma once

#include <glow/common/shared.hh>
#include <glow/gl.hh>

namespace glow
{
GLOW_SHARED(class, ArrayBuffer);

/// A location in this VAO
struct VertexArrayAttribute
{
    SharedArrayBuffer buffer;
    GLuint locationInBuffer;
    GLuint locationInVAO;
};
}
