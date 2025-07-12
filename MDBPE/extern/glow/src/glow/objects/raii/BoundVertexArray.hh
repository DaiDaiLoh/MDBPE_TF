#pragma once

#include <initializer_list>

#include <clean-core/span.hh>

#include <glow/common/non_copyable.hh>
#include <glow/fwd.hh>
#include <glow/gl.hh>

namespace glow
{
/// RAII-object that defines a "bind"-scope for a VertexArray
/// All functions that operate on the currently bound VAO are accessed here
struct BoundVertexArray
{
    GLOW_RAII_CLASS(BoundVertexArray);

    /// Backreference to the program
    VertexArray* const vao;

public: // gl function that require binding
    /// Draws the VAO
    /// Requires a currently used shader (otherwise: runtime error)
    /// Automatically determines index and unindexed drawing
    /// Uses the first divisor-0 array buffer to determine number of primitives
    /// Negotiates location mappings IF current program != nullptr
    /// If instanceCount is negative, getInstanceCount() is used.
    void draw(GLsizei instanceCount = -1);
    /// Same as draw(...) but only renders a subrange of indices or vertices
    void drawRange(GLsizei start, GLsizei end, GLsizei instanceCount = -1);
    /// Same as draw(...) but takes the number of vertices from a recorded transform feedback object
    /// NOTE: does not work with index buffers or instancing
    void drawTransformFeedback(SharedTransformFeedback const& feedback);

    /// Attaches an element array buffer
    /// Overrides the previously attached one
    /// nullptr is a valid argument
    void attach(SharedElementArrayBuffer const& eab);

    /// Attaches all attributes of the given array buffer
    /// Override attributes with the same name
    /// nullptr is NOT a valid argument
    void attach(SharedArrayBuffer const& ab);

    /// Attaches all attributes of the given array buffers
    /// Override attributes with the same name
    void attach(cc::span<SharedArrayBuffer const> abs);
    void attach(std::initializer_list<SharedArrayBuffer> abs) { attach(cc::span(abs.begin(), abs.size())); }

    /// Re-attaches array buffers with current locations
    /// Probably not required by an end-user
    void reattach();

    /// Negotiates attribute bindings with the currently bound program
    /// Is called automatically on use
    /// The only time you need this manually is when using transform feedback
    void negotiateBindings();

private:
    GLint previousVAO;                ///< previously used vao
    GLint previousEAB;                ///< previously used eab
    BoundVertexArray* previousVaoPtr; ///< previously used vao
    BoundVertexArray(VertexArray* vao);
    friend class VertexArray;

    /// returns true iff it's safe to use this bound class
    /// otherwise, runtime error
    bool isCurrent() const;

    /// updates patch parameters if GL_PATCHES is used
    void updatePatchParameters();

public:
    BoundVertexArray(BoundVertexArray&&); // allow move
    ~BoundVertexArray();
};
}
