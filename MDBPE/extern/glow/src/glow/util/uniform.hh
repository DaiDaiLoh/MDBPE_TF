#pragma once

#include <initializer_list>
#include <string_view>

#include <glow/common/array_view.hh>
#include <glow/common/macro_join.hh>
#include <glow/fwd.hh>
#include <glow/gl.hh>

#ifdef GLOW_HAS_GLM
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>
#endif

#include <typed-geometry/tg-lean.hh>

namespace glow
{
struct UsedProgram;
}

namespace glow::detail
{
/**
 * Proxy class for low overhead uniform setting
 *
 * TODO: reintroduce size checking for array types
 * TODO: check compile times and see if partial specialization would be better
 * TODO: auto-conversion for bool vector assignments
 * TODO: clean up all the templates by delegating to a few common ones
 */
template <class T>
struct uniform
{
    static constexpr bool is_supported = false;

    template <class U = T>
    uniform(UsedProgram&, U&&)
    {
        static_assert(tg::always_false<U>, "this type is not supported for uniforms");
    }
};

// TODO: only save location and expected type
// CAUTION: do not store is class
struct uniform_proxy
{
    explicit uniform_proxy(UsedProgram& prog, std::string_view name) : prog(prog), name(name) {}

    template <class T>
    void operator=(T const& v) &&;
    template <class T>
    void operator=(std::initializer_list<T> v) &&;
    template <class TextureT>
    void operator=(std::shared_ptr<TextureT> const& tex) &&;
    template <class TextureT>
    void operator=(AsyncTexture<TextureT> const& tex) &&;
    template <class T>
    void operator=(T const&) &
    {
        static_assert(tg::always_false<T>, "this class must not be stored");
    }

private:
    UsedProgram& prog;
    std::string_view name;
};

struct uniform_base
{
    static constexpr bool is_supported = true;

    GLint location = -1;
    uniform_base(GLint loc) : location(loc) {}
    uniform_base(UsedProgram& prog, std::string_view name, int size, GLenum type);
};

// basic types
template <>
struct uniform<bool> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_BOOL) {}
    void operator=(bool v) const { glUniform1i(location, v); }
};
template <>
struct uniform<int32_t> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_INT) {}
    void operator=(int32_t v) const { glUniform1i(location, v); }
};
template <>
struct uniform<uint32_t> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_UNSIGNED_INT) {}
    void operator=(uint32_t v) const { glUniform1ui(location, v); }
};
template <>
struct uniform<float> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT) {}
    void operator=(float v) const { glUniform1f(location, v); }
};
template <>
struct uniform<double> : uniform_base
{
    template <class T>
    uniform(UsedProgram&, T&&)
    {
        static_assert(tg::always_false<T>, "there is no API to set double uniforms and it's a bad idea anyways (pretty slow)");
    }
};

// basic array types
template <>
struct uniform<bool[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_BOOL) {}
    void operator=(array_view<const bool> v) const;
    void operator=(array_view<const int32_t> v) const { glUniform1iv(location, int(v.size()), v.data()); }
    void operator=(array_view<const uint32_t> v) const { glUniform1uiv(location, int(v.size()), v.data()); }
    void operator=(array_view<const float> v) const { glUniform1fv(location, int(v.size()), v.data()); }
};
template <>
struct uniform<int32_t[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_INT) {}
    void operator=(array_view<const int32_t> v) const { glUniform1iv(location, int(v.size()), v.data()); }
};
template <>
struct uniform<uint32_t[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_UNSIGNED_INT) {}
    void operator=(array_view<const uint32_t> v) const { glUniform1uiv(location, int(v.size()), v.data()); }
};
template <>
struct uniform<float[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT) {}
    void operator=(array_view<const float> v) const { glUniform1fv(location, int(v.size()), v.data()); }
};

// non-array types
#define GLOW_IMPL_COMP_UNIFORM_NO_ARRAY(ns, type, base_type, gl_type, uniform_suffix)                                                               \
    template <>                                                                                                                                     \
    struct uniform<ns::type##2> : uniform_base                                                                                                      \
    {                                                                                                                                               \
        uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_##gl_type##_VEC2) {}                                     \
        void operator=(ns::type##2 const& v) const                                                                                                  \
        {                                                                                                                                           \
            glUniform##2##uniform_suffix(location, v.TG_IMPL_MEMBER(base_type, 0), v.TG_IMPL_MEMBER(base_type, 1));                                 \
        }                                                                                                                                           \
    };                                                                                                                                              \
    template <>                                                                                                                                     \
    struct uniform<ns::type##3> : uniform_base                                                                                                      \
    {                                                                                                                                               \
        uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_##gl_type##_VEC3) {}                                     \
        void operator=(ns::type##3 const& v) const                                                                                                  \
        {                                                                                                                                           \
            glUniform##3##uniform_suffix(location, v.TG_IMPL_MEMBER(base_type, 0), v.TG_IMPL_MEMBER(base_type, 1), v.TG_IMPL_MEMBER(base_type, 2)); \
        }                                                                                                                                           \
    };                                                                                                                                              \
    template <>                                                                                                                                     \
    struct uniform<ns::type##4> : uniform_base                                                                                                      \
    {                                                                                                                                               \
        uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_##gl_type##_VEC4) {}                                     \
        void operator=(ns::type##4 const& v) const                                                                                                  \
        {                                                                                                                                           \
            glUniform##4##uniform_suffix(location, v.TG_IMPL_MEMBER(base_type, 0), v.TG_IMPL_MEMBER(base_type, 1), v.TG_IMPL_MEMBER(base_type, 2),  \
                                         v.TG_IMPL_MEMBER(base_type, 3));                                                                           \
        }                                                                                                                                           \
    } // force ;

#define GLOW_IMPL_COMP_UNIFORM(ns, type, base_type, gl_type, uniform_suffix, c_type)                             \
    GLOW_IMPL_COMP_UNIFORM_NO_ARRAY(ns, type, base_type, gl_type, uniform_suffix);                               \
    template <>                                                                                                  \
    struct uniform<ns::type##2 []> : uniform_base                                                                \
    {                                                                                                            \
        uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_##gl_type##_VEC2) {} \
        void operator=(array_view<const ns::type##2> v) const                                                    \
        {                                                                                                        \
            glUniform##2##uniform_suffix##v(location, int(v.size()), reinterpret_cast<c_type const*>(v.data())); \
        }                                                                                                        \
    };                                                                                                           \
    template <>                                                                                                  \
    struct uniform<ns::type##3 []> : uniform_base                                                                \
    {                                                                                                            \
        uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_##gl_type##_VEC3) {} \
        void operator=(array_view<const ns::type##3> v) const                                                    \
        {                                                                                                        \
            glUniform##3##uniform_suffix##v(location, int(v.size()), reinterpret_cast<c_type const*>(v.data())); \
        }                                                                                                        \
    };                                                                                                           \
    template <>                                                                                                  \
    struct uniform<ns::type##4 []> : uniform_base                                                                \
    {                                                                                                            \
        uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_##gl_type##_VEC3) {} \
        void operator=(array_view<const ns::type##4> v) const                                                    \
        {                                                                                                        \
            glUniform##4##uniform_suffix##v(location, int(v.size()), reinterpret_cast<c_type const*>(v.data())); \
        }                                                                                                        \
    } // force ;

// glm
#ifdef GLOW_HAS_GLM
GLOW_IMPL_COMP_UNIFORM(glm, vec, vec, FLOAT, f, float);
GLOW_IMPL_COMP_UNIFORM(glm, ivec, vec, INT, i, GLint);
GLOW_IMPL_COMP_UNIFORM(glm, uvec, vec, UNSIGNED_INT, ui, GLuint);

// bool vectors
template <>
struct uniform<glm::bvec2> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_BOOL_VEC2) {}
    void operator=(glm::bvec2 const& v) const { glUniform2i(location, int(v.x), int(v.y)); }
};
template <>
struct uniform<glm::bvec3> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_BOOL_VEC3) {}
    void operator=(glm::bvec3 const& v) const { glUniform3i(location, int(v.x), int(v.y), int(v.z)); }
};
template <>
struct uniform<glm::bvec4> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_BOOL_VEC4) {}
    void operator=(glm::bvec4 const& v) const { glUniform4i(location, int(v.x), int(v.y), int(v.z), int(v.w)); }
};
template <>
struct uniform<glm::bvec2[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_BOOL_VEC2) {}
    void operator=(array_view<const tg::comp2> v) const { glUniform2fv(location, int(v.size()), reinterpret_cast<GLfloat const*>(v.data())); }
    void operator=(array_view<const tg::icomp2> v) const { glUniform2iv(location, int(v.size()), reinterpret_cast<GLint const*>(v.data())); }
    void operator=(array_view<const tg::ucomp2> v) const { glUniform2uiv(location, int(v.size()), reinterpret_cast<GLuint const*>(v.data())); }
};
template <>
struct uniform<glm::bvec3[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_BOOL_VEC3) {}
    void operator=(array_view<const tg::comp3> v) const { glUniform3fv(location, int(v.size()), reinterpret_cast<GLfloat const*>(v.data())); }
    void operator=(array_view<const tg::icomp3> v) const { glUniform3iv(location, int(v.size()), reinterpret_cast<GLint const*>(v.data())); }
    void operator=(array_view<const tg::ucomp3> v) const { glUniform3uiv(location, int(v.size()), reinterpret_cast<GLuint const*>(v.data())); }
};
template <>
struct uniform<glm::bvec4[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_BOOL_VEC4) {}
    void operator=(array_view<const tg::comp4> v) const { glUniform4fv(location, int(v.size()), reinterpret_cast<GLfloat const*>(v.data())); }
    void operator=(array_view<const tg::icomp4> v) const { glUniform4iv(location, int(v.size()), reinterpret_cast<GLint const*>(v.data())); }
    void operator=(array_view<const tg::ucomp4> v) const { glUniform4uiv(location, int(v.size()), reinterpret_cast<GLuint const*>(v.data())); }
};

// matrices
template <>
struct uniform<glm::mat2> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT2) {}
    void operator=(glm::mat2 const& v) const { glUniformMatrix2fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<glm::mat3> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT3) {}
    void operator=(glm::mat3 const& v) const { glUniformMatrix3fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<glm::mat4> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT4) {}
    void operator=(glm::mat4 const& v) const { glUniformMatrix4fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<glm::mat2x3> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT2x3) {}
    void operator=(glm::mat2x3 const& v) const { glUniformMatrix2x3fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<glm::mat2x4> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT2x4) {}
    void operator=(glm::mat2x4 const& v) const { glUniformMatrix2x4fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<glm::mat3x2> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT3x2) {}
    void operator=(glm::mat3x2 const& v) const { glUniformMatrix3x2fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<glm::mat3x4> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT3x4) {}
    void operator=(glm::mat3x4 const& v) const { glUniformMatrix3x4fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<glm::mat4x2> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT4x2) {}
    void operator=(glm::mat4x2 const& v) const { glUniformMatrix4x2fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<glm::mat4x3> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT4x3) {}
    void operator=(glm::mat4x3 const& v) const { glUniformMatrix4x3fv(location, 1, GL_FALSE, &v[0][0]); }
};

// arrays of matrices
template <>
struct uniform<glm::mat2[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT2) {}
    void operator=(array_view<const glm::mat2> v) const
    {
        glUniformMatrix2fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<glm::mat3[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT3) {}
    void operator=(array_view<const glm::mat3> v) const
    {
        glUniformMatrix3fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<glm::mat4[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT4) {}
    void operator=(array_view<const glm::mat4> v) const
    {
        glUniformMatrix4fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<glm::mat2x3[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT2x3) {}
    void operator=(array_view<const glm::mat2x3> v) const
    {
        glUniformMatrix2x3fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<glm::mat2x4[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT2x4) {}
    void operator=(array_view<const glm::mat2x4> v) const
    {
        glUniformMatrix2x4fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<glm::mat3x2[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT3x2) {}
    void operator=(array_view<const glm::mat3x2> v) const
    {
        glUniformMatrix3x2fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<glm::mat3x4[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT3x4) {}
    void operator=(array_view<const glm::mat3x4> v) const
    {
        glUniformMatrix3x4fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<glm::mat4x2[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT4x2) {}
    void operator=(array_view<const glm::mat4x2> v) const
    {
        glUniformMatrix4x2fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<glm::mat4x3[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT4x3) {}
    void operator=(array_view<const glm::mat4x3> v) const
    {
        glUniformMatrix4x3fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
#endif

// tg
GLOW_IMPL_COMP_UNIFORM(tg, vec, vec, FLOAT, f, float);
GLOW_IMPL_COMP_UNIFORM(tg, ivec, vec, INT, i, GLint);
GLOW_IMPL_COMP_UNIFORM(tg, uvec, vec, UNSIGNED_INT, ui, GLuint);

GLOW_IMPL_COMP_UNIFORM(tg, pos, pos, FLOAT, f, float);
GLOW_IMPL_COMP_UNIFORM(tg, ipos, pos, INT, i, GLint);
GLOW_IMPL_COMP_UNIFORM(tg, upos, pos, UNSIGNED_INT, ui, GLuint);

GLOW_IMPL_COMP_UNIFORM(tg, size, size, FLOAT, f, float);
GLOW_IMPL_COMP_UNIFORM(tg, isize, size, INT, i, GLint);
GLOW_IMPL_COMP_UNIFORM(tg, usize, size, UNSIGNED_INT, ui, GLuint);

GLOW_IMPL_COMP_UNIFORM(tg, dir, dir, FLOAT, f, float);
GLOW_IMPL_COMP_UNIFORM(tg, idir, dir, INT, i, GLint);

GLOW_IMPL_COMP_UNIFORM(tg, comp, comp, FLOAT, f, float);
GLOW_IMPL_COMP_UNIFORM(tg, icomp, comp, INT, i, GLint);
GLOW_IMPL_COMP_UNIFORM(tg, ucomp, comp, UNSIGNED_INT, ui, GLuint);

// bool vectors
template <>
struct uniform<tg::bcomp2> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_BOOL_VEC2) {}
    void operator=(tg::bcomp2 const& v) const { glUniform2i(location, int(v.comp0), int(v.comp1)); }
};
template <>
struct uniform<tg::bcomp3> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_BOOL_VEC3) {}
    void operator=(tg::bcomp3 const& v) const { glUniform3i(location, int(v.comp0), int(v.comp1), int(v.comp2)); }
};
template <>
struct uniform<tg::bcomp4> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_BOOL_VEC4) {}
    void operator=(tg::bcomp4 const& v) const { glUniform4i(location, int(v.comp0), int(v.comp1), int(v.comp2), int(v.comp3)); }
};
template <>
struct uniform<tg::bcomp2[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_BOOL_VEC2) {}
    void operator=(array_view<const tg::comp2> v) const { glUniform2fv(location, int(v.size()), reinterpret_cast<GLfloat const*>(v.data())); }
    void operator=(array_view<const tg::icomp2> v) const { glUniform2iv(location, int(v.size()), reinterpret_cast<GLint const*>(v.data())); }
    void operator=(array_view<const tg::ucomp2> v) const { glUniform2uiv(location, int(v.size()), reinterpret_cast<GLuint const*>(v.data())); }
};
template <>
struct uniform<tg::bcomp3[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_BOOL_VEC3) {}
    void operator=(array_view<const tg::comp3> v) const { glUniform3fv(location, int(v.size()), reinterpret_cast<GLfloat const*>(v.data())); }
    void operator=(array_view<const tg::icomp3> v) const { glUniform3iv(location, int(v.size()), reinterpret_cast<GLint const*>(v.data())); }
    void operator=(array_view<const tg::ucomp3> v) const { glUniform3uiv(location, int(v.size()), reinterpret_cast<GLuint const*>(v.data())); }
};
template <>
struct uniform<tg::bcomp4[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_BOOL_VEC4) {}
    void operator=(array_view<const tg::comp4> v) const { glUniform4fv(location, int(v.size()), reinterpret_cast<GLfloat const*>(v.data())); }
    void operator=(array_view<const tg::icomp4> v) const { glUniform4iv(location, int(v.size()), reinterpret_cast<GLint const*>(v.data())); }
    void operator=(array_view<const tg::ucomp4> v) const { glUniform4uiv(location, int(v.size()), reinterpret_cast<GLuint const*>(v.data())); }
};

// matrices
template <>
struct uniform<tg::mat2> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT2) {}
    void operator=(tg::mat2 const& v) const { glUniformMatrix2fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<tg::mat3> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT3) {}
    void operator=(tg::mat3 const& v) const { glUniformMatrix3fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<tg::mat4> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT4) {}
    void operator=(tg::mat4 const& v) const { glUniformMatrix4fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<tg::mat2x3> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT2x3) {}
    void operator=(tg::mat2x3 const& v) const { glUniformMatrix2x3fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<tg::mat2x4> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT2x4) {}
    void operator=(tg::mat2x4 const& v) const { glUniformMatrix2x4fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<tg::mat3x2> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT3x2) {}
    void operator=(tg::mat3x2 const& v) const { glUniformMatrix3x2fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<tg::mat3x4> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT3x4) {}
    void operator=(tg::mat3x4 const& v) const { glUniformMatrix3x4fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<tg::mat4x2> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT4x2) {}
    void operator=(tg::mat4x2 const& v) const { glUniformMatrix4x2fv(location, 1, GL_FALSE, &v[0][0]); }
};
template <>
struct uniform<tg::mat4x3> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_MAT4x3) {}
    void operator=(tg::mat4x3 const& v) const { glUniformMatrix4x3fv(location, 1, GL_FALSE, &v[0][0]); }
};

// arrays of matrices
template <>
struct uniform<tg::mat2[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT2) {}
    void operator=(array_view<const tg::mat2> v) const
    {
        glUniformMatrix2fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<tg::mat3[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT3) {}
    void operator=(array_view<const tg::mat3> v) const
    {
        glUniformMatrix3fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<tg::mat4[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT4) {}
    void operator=(array_view<const tg::mat4> v) const
    {
        glUniformMatrix4fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<tg::mat2x3[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT2x3) {}
    void operator=(array_view<const tg::mat2x3> v) const
    {
        glUniformMatrix2x3fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<tg::mat2x4[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT2x4) {}
    void operator=(array_view<const tg::mat2x4> v) const
    {
        glUniformMatrix2x4fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<tg::mat3x2[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT3x2) {}
    void operator=(array_view<const tg::mat3x2> v) const
    {
        glUniformMatrix3x2fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<tg::mat3x4[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT3x4) {}
    void operator=(array_view<const tg::mat3x4> v) const
    {
        glUniformMatrix3x4fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<tg::mat4x2[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT4x2) {}
    void operator=(array_view<const tg::mat4x2> v) const
    {
        glUniformMatrix4x2fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};
template <>
struct uniform<tg::mat4x3[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT_MAT4x3) {}
    void operator=(array_view<const tg::mat4x3> v) const
    {
        glUniformMatrix4x3fv(location, int(v.size()), GL_FALSE, reinterpret_cast<float const*>(v.data()));
    }
};

// colors
template <>
struct uniform<tg::color3> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_VEC3) {}
    void operator=(tg::color3 const& v) const { glUniform3f(location, v.r, v.g, v.b); }
};
template <>
struct uniform<tg::color4> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, 1, GL_FLOAT_VEC4) {}
    void operator=(tg::color4 const& v) const { glUniform4f(location, v.r, v.g, v.b, v.a); }
};
template <>
struct uniform<tg::color3[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT) {}
    void operator=(array_view<const tg::color3> v) const { glUniform3fv(location, int(v.size()), reinterpret_cast<float const*>(v.data())); }
};
template <>
struct uniform<tg::color4[]> : uniform_base
{
    uniform(UsedProgram& prog, std::string_view name) : uniform_base(prog, name, -1, GL_FLOAT) {}
    void operator=(array_view<const tg::color4> v) const { glUniform4fv(location, int(v.size()), reinterpret_cast<float const*>(v.data())); }
};

#undef GLOW_IMPL_COMP_UNIFORM
#undef GLOW_IMPL_COMP_UNIFORM_NO_ARRAY
}
