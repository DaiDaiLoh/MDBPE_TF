#include "Sampler.hh"

#include <typed-geometry/feature/assert.hh>

#include <limits>

#include <glow/glow.hh>

namespace glow
{
Sampler::Sampler()
{
    checkValidGLOW();

    mObjectName = std::numeric_limits<decltype(mObjectName)>::max();
    glGenSamplers(1, &mObjectName);
    TG_ASSERT(mObjectName != std::numeric_limits<decltype(mObjectName)>::max() && "No OpenGL Context?");
}

Sampler::~Sampler() { glDeleteSamplers(1, &mObjectName); }

void Sampler::setWrap(GLenum wrapS, GLenum wrapT, GLenum wrapR)
{
    setWrapS(wrapS);
    setWrapT(wrapT);
    setWrapR(wrapR);
}

void Sampler::setFilter(GLenum minFilter, GLenum magFilter)
{
    setMinFilter(minFilter);
    setMagFilter(magFilter);
}

void Sampler::setMinFilter(GLenum value)
{
    checkValidGLOW();
    glSamplerParameteri(mObjectName, GL_TEXTURE_MIN_FILTER, value);
}

GLenum Sampler::getMinFilter()
{
    checkValidGLOW();
    GLint value;
    glGetSamplerParameteriv(mObjectName, GL_TEXTURE_MIN_FILTER, &value);
    return value;
}

void Sampler::setMagFilter(GLenum value)
{
    checkValidGLOW();
    glSamplerParameteri(mObjectName, GL_TEXTURE_MAG_FILTER, value);
}

GLenum Sampler::getMagFilter()
{
    checkValidGLOW();
    GLint value;
    glGetSamplerParameteriv(mObjectName, GL_TEXTURE_MAG_FILTER, &value);
    return value;
}

void Sampler::setWrapS(GLenum value)
{
    checkValidGLOW();
    glSamplerParameteri(mObjectName, GL_TEXTURE_WRAP_S, value);
}

GLenum Sampler::getWrapS()
{
    checkValidGLOW();
    GLint value;
    glGetSamplerParameteriv(mObjectName, GL_TEXTURE_WRAP_S, &value);
    return value;
}

void Sampler::setWrapR(GLenum value)
{
    checkValidGLOW();
    glSamplerParameteri(mObjectName, GL_TEXTURE_WRAP_R, value);
}

GLenum Sampler::getWrapR()
{
    checkValidGLOW();
    GLint value;
    glGetSamplerParameteriv(mObjectName, GL_TEXTURE_WRAP_R, &value);
    return value;
}

void Sampler::setMinLOD(float value)
{
    checkValidGLOW();
    glSamplerParameterf(mObjectName, GL_TEXTURE_MIN_LOD, value);
}

float Sampler::getMinLOD()
{
    checkValidGLOW();
    float value;
    glGetSamplerParameterfv(mObjectName, GL_TEXTURE_MIN_LOD, &value);
    return value;
}

void Sampler::setMaxLOD(float value)
{
    checkValidGLOW();
    glSamplerParameterf(mObjectName, GL_TEXTURE_MAX_LOD, value);
}

float Sampler::getMaxLOD()
{
    checkValidGLOW();
    float value;
    glGetSamplerParameterfv(mObjectName, GL_TEXTURE_MAX_LOD, &value);
    return value;
}

void Sampler::setBorderColor(tg::color3 value) { setBorderColor(tg::color4(value, 1.0f)); }

void Sampler::setBorderColor(tg::color4 value)
{
    checkValidGLOW();
    glSamplerParameterfv(mObjectName, GL_TEXTURE_BORDER_COLOR, &value[0]);
}

tg::color4 Sampler::getBorderColor()
{
    checkValidGLOW();
    tg::color4 value;
    glGetSamplerParameterfv(mObjectName, GL_TEXTURE_BORDER_COLOR, &value[0]);
    return value;
}

void Sampler::setWrapT(GLenum value)
{
    checkValidGLOW();
    glSamplerParameteri(mObjectName, GL_TEXTURE_WRAP_T, value);
}

GLenum Sampler::getWrapT()
{
    checkValidGLOW();
    GLint value;
    glGetSamplerParameteriv(mObjectName, GL_TEXTURE_WRAP_T, &value);
    return value;
}

void Sampler::setCompareMode(GLenum value)
{
    checkValidGLOW();
    glSamplerParameteri(mObjectName, GL_TEXTURE_COMPARE_MODE, value);
}

GLenum Sampler::getCompareMode()
{
    checkValidGLOW();
    GLint value;
    glGetSamplerParameteriv(mObjectName, GL_TEXTURE_COMPARE_MODE, &value);
    return value;
}

void Sampler::setCompareFunc(GLenum value)
{
    checkValidGLOW();
    glSamplerParameteri(mObjectName, GL_TEXTURE_COMPARE_FUNC, value);
}

GLenum Sampler::getCompareFunc()
{
    checkValidGLOW();
    GLint value;
    glGetSamplerParameteriv(mObjectName, GL_TEXTURE_COMPARE_FUNC, &value);
    return value;
}

SharedSampler Sampler::create() { return std::make_shared<Sampler>(); }
}
