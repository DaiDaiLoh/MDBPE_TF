#pragma once

#include <typed-geometry/tg-lean.hh>

#include <glow/common/shared.hh>
#include <glow/gl.hh>

#include "NamedObject.hh"

namespace glow
{
GLOW_SHARED(class, Sampler);

/**
 * @brief Samplers encapsulate texture parameters like filtering or max LOD
 *
 * Samplers do not need to be bound in order to change its settings
 *
 * Usage:
 *  auto mySampler = Sampler::create();
 *  mySampler->setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
 *
 *  auto shader = myProgram->use();
 *  shader.setTexture("uTex", myTexture, mySampler);
 */
class Sampler final : public NamedObject<Sampler, GL_SAMPLER>
{
private:
    GLuint mObjectName;

public:
    /// returns the raw object name to be used directly in OpenGL functions
    GLuint getObjectName() const { return mObjectName; }

public:
    Sampler();
    ~Sampler();

    // aggregate states
public:
    void setWrap(GLenum wrapS, GLenum wrapT = GL_REPEAT, GLenum wrapR = GL_REPEAT);
    void setFilter(GLenum minFilter, GLenum magFilter);

    // individual states
public:
    void setMinFilter(GLenum value);
    GLenum getMinFilter();

    void setMagFilter(GLenum value);
    GLenum getMagFilter();

    void setWrapS(GLenum value);
    GLenum getWrapS();
    void setWrapT(GLenum value);
    GLenum getWrapT();
    void setWrapR(GLenum value);
    GLenum getWrapR();

    void setMinLOD(float value);
    float getMinLOD();
    void setMaxLOD(float value);
    float getMaxLOD();

    void setBorderColor(tg::color3 value);
    void setBorderColor(tg::color4 value);
    tg::color4 getBorderColor();

    void setCompareMode(GLenum value);
    GLenum getCompareMode();

    void setCompareFunc(GLenum value);
    GLenum getCompareFunc();

public:
    /// creates a new sampler in default state
    static SharedSampler create();
};
}
