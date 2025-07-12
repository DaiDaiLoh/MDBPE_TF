#pragma once

#include <string_view>

#include <typed-geometry/tg-lean.hh>

#include <glow/common/force_semicolon.hh>
#include <glow/common/macro_join.hh>
#include <glow/common/non_copyable.hh>
#include <glow/fwd.hh>
#include <glow/gl.hh>
#include <glow/util/uniform.hh>

namespace glow
{
GLOW_SHARED(class, UniformState);

/// RAII-object that defines a "use"-scope for a Program
/// All functions that operate on the currently bound program are accessed here
struct UsedProgram
{
    GLOW_RAII_CLASS(UsedProgram);

    /// Backreference to the program
    Program* const program;

public: // gl functions with use
    /// Binds a texture to a uniform
    /// Automatically chooses a free texture unit starting from 0
    /// Setting nullptr is ok
    /// Optionally specifies a sampler for this texture
    void setTexture(std::string_view name, SharedTexture const& tex, SharedSampler const& sampler = nullptr);
    /// Binds a texture to an image sampler
    /// Requires an explicit binding location in shader (e.g. binding=N layout)
    /// Requires tex->isStorageImmutable()
    void setImage(int bindingLocation, SharedTexture const& tex, GLenum usage = GL_READ_WRITE, int mipmapLevel = 0, int layer = 0);

    /// returns a small proxy object that can be used to set uniforms with minimal overhead
    /// Usage:
    ///   auto shader = mProgram->use();
    ///   auto u = shader.uniform<float>("uMyFloat");
    ///   // .. inner loop:
    ///   u = 17.f;
    ///   shader.uniform<int[]>("uMyIntArray") = {1, 2, 3};
    ///
    /// Alternative usage:
    ///   shader.setUniform("uMyFloat", 17.f);
    ///
    /// supported types:
    ///   bool
    ///   int
    ///   float
    ///   unsigned
    ///   [ iub]vec[234]
    ///   mat[234]
    ///   mat[234]x[234]
    ///   .. and arrays of them
    ///
    /// NOTE: this objects should not outlive the UsedProgram (relinking might invalidate it)
    /// NOTE: there are no double uniforms!
    template <class T>
    detail::uniform<T> uniform(std::string_view name)
    {
        return detail::uniform<T>(*this, name);
    }

    // shorthand for uniform<T>(name) = value;
    // also translates array-like types to T[]
    template <class T>
    void setUniform(std::string_view name, T const& value)
    {
        if constexpr (detail::uniform<T>::is_supported)
            uniform<T>(name) = value;
        else if constexpr (std::is_array_v<T>)
        {
            using E = std::decay_t<decltype(value[0])>;
            uniform<E[]>(name) = value;
        }
        else if constexpr (detail::can_make_array_view<T>)
        {
            using E = std::decay_t<decltype(value.data()[0])>;
            auto view = array_view<E const>(value);
            uniform<E[]>(name) = view;
        }
        else
            static_assert(tg::always_false<T>, "don't know how to convert type to uniform (or uniform array)");
    }
    template <class T>
    void setUniform(std::string_view name, std::initializer_list<T> value)
    {
        uniform<T[]>(name) = value;
    }

    // returns an untyped object that can be used to assign textures and uniforms
    // Example:
    //    shader["uMyInt"] = 7;
    //    shader["uMyTex"] = myTexture;
    detail::uniform_proxy operator[](std::string_view name) { return detail::uniform_proxy(*this, name); }

    /// Applies all saved uniforms from that state
    void setUniforms(SharedUniformState const& state);
    /// Sets generic uniform data
    /// CAUTION: bool is assumed as glUniformi (i.e. integer)
    void setUniform(std::string_view name, GLenum uniformType, GLint size, void const* data);
    // ========================================== UNIFORMS - END ==========================================


    /// Invokes the compute shader with the given number of groups
    void compute(GLuint groupsX = 1, GLuint groupsY = 1, GLuint groupsZ = 1);
    void compute(tg::isize2 const& groups) { compute(groups.width, groups.height); }
    void compute(tg::isize3 const& groups) { compute(groups.width, groups.height, groups.depth); }

    /// Starts transform feedback
    /// feedbackBuffer is an optional buffer that is automatically bound to GL_TRANSFORM_FEEDBACK_BUFFER
    /// NOTE: it is recommended to use a FeedbackObject
    void beginTransformFeedback(GLenum primitiveMode, SharedBuffer const& feedbackBuffer = nullptr);
    /// Ends transform feedback
    void endTransformFeedback();

    /// Check for shader reloading
    void checkShaderReload();

private:
    /// Special case: bool uniforms
    void setUniformBool(std::string_view name, int count, int32_t const* values) const;

    /// Special case: overwrite uniform type
    void setUniformIntInternal(std::string_view name, int count, int32_t const* values, GLenum uniformType) const;

private:
    GLint previousProgram;           ///< previously used program
    UsedProgram* previousProgramPtr; ///< previously used program
    UsedProgram(Program* program);
    friend class Program;

    /// returns true iff it's safe to use this bound class
    /// otherwise, runtime error
    bool isCurrent() const;

public:
    UsedProgram(UsedProgram&&); // allow move
    ~UsedProgram();

    friend detail::uniform_base;
};

namespace detail
{
template <class T>
void uniform_proxy::operator=(T const& v) &&
{
    prog.setUniform(name, v);
}
template <class T>
void uniform_proxy::operator=(std::initializer_list<T> v) &&
{
    prog.setUniform(name, v);
}
template <class TextureT>
void uniform_proxy::operator=(std::shared_ptr<TextureT> const& tex) &&
{
    prog.setTexture(name, tex);
}
template <class TextureT>
void uniform_proxy::operator=(AsyncTexture<TextureT> const& tex) &&
{
    prog.setTexture(name, tex.texture());
}
}
}
