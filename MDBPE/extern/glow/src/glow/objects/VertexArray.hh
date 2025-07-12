#pragma once

#include <string_view>
#include <vector>

#include <glow/common/nodiscard.hh>
#include <glow/common/non_copyable.hh>
#include <glow/common/shared.hh>

#include <glow/gl.hh>

#include "NamedObject.hh"
#include "VertexArrayAttribute.hh"
#include "raii/BoundVertexArray.hh"


namespace glow
{
GLOW_SHARED(class, ArrayBuffer);
GLOW_SHARED(class, ElementArrayBuffer);
GLOW_SHARED(class, VertexArray);
GLOW_SHARED(class, LocationMapping);
GLOW_SHARED(class, TransformFeedback);

class VertexArray final : public NamedObject<VertexArray, GL_VERTEX_ARRAY>
{
private:
    /// OGL id
    GLuint mObjectName;

    /// OGL primitive mode
    GLenum mPrimitiveMode;
    /// Number of vertices per patch (for primitive mode GL_PATCHES)
    int mVerticesPerPatch = -1;

    /// Attached element array buffer
    SharedElementArrayBuffer mElementArrayBuffer;

    /// ArrayBuffer Attributes
    std::vector<VertexArrayAttribute> mAttributes;

    /// Location mapping from attribute name to loc
    /// At any point, the mapping saved here must be consistent (i.e. a superset) of the GPU mapping
    SharedLocationMapping mAttributeMapping;

private:
    /// Attaches the given attribute to the current VAO
    static void attachAttribute(VertexArrayAttribute const& a);

public: // getter
    GLuint getObjectName() const { return mObjectName; }
    GLenum getPrimitiveMode() const { return mPrimitiveMode; }
    void setPrimitiveMode(GLenum mode) { mPrimitiveMode = mode; }
    void setVerticesPerPatch(int cnt) { mVerticesPerPatch = cnt; }
    SharedLocationMapping const& getAttributeMapping() const { return mAttributeMapping; }
    SharedElementArrayBuffer const& getElementArrayBuffer() const { return mElementArrayBuffer; }
    std::vector<VertexArrayAttribute> const& getAttributes() const { return mAttributes; }

    /// returns true iff a draw call would draw zero primitives
    bool isEmpty() const;

public:
    /// returns the AB that contains the given attribute (or nullptr if not found)
    SharedArrayBuffer getAttributeBuffer(std::string_view name) const;
    /// returns the number of instances that this VertexArray should logically draw
    /// is 1 if no divisor > 0 is present
    /// otherwise max of divisor * elementCount per attribute
    int getInstanceCount() const;
    /// returns the number of primitives in the first divisor-0 array buffer
    /// returns 0 if none present
    int getVertexCount() const;

public:
    /// Gets the currently bound VAO (nullptr if none)
    static BoundVertexArray* getCurrentVAO();

public:
    VertexArray(GLenum primitiveMode = GL_TRIANGLES);
    ~VertexArray();


    /// Binds this vertex array.
    /// Unbinding is done when the returned object runs out of scope.
    /// CAUTION: Cannot be used while an EAB is bound! (runtime error)
    GLOW_NODISCARD BoundVertexArray bind() { return {this}; }
    friend BoundVertexArray;

public: // static construction
    /// creates an empty VAO
    /// same as std::make_shared<VertexArray>()
    static SharedVertexArray create(GLenum primitiveMode = GL_TRIANGLES);
    /// creates a VAO and attaches all specified attributes (and optionally an EAB)
    static SharedVertexArray create(SharedArrayBuffer const& ab, SharedElementArrayBuffer const& eab, GLenum primitiveMode = GL_TRIANGLES);
    static SharedVertexArray create(SharedArrayBuffer const& ab, GLenum primitiveMode = GL_TRIANGLES);
    /// creates a VAO and attaches all specified attributes (and optionally an EAB)
    static SharedVertexArray create(cc::span<SharedArrayBuffer const> abs, SharedElementArrayBuffer const& eab = nullptr, GLenum primitiveMode = GL_TRIANGLES);
    static SharedVertexArray create(std::initializer_list<SharedArrayBuffer> abs, SharedElementArrayBuffer const& eab = nullptr, GLenum primitiveMode = GL_TRIANGLES)
    {
        return create(cc::span(abs.begin(), abs.size()), eab, primitiveMode);
    }
};
}
