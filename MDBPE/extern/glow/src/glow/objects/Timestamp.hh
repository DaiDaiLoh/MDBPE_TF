#pragma once

#include <glow/common/shared.hh>
#include <glow/gl.hh>

#include "NamedObject.hh"

namespace glow
{
GLOW_SHARED(class, Timestamp);

class Timestamp : public NamedObject<Timestamp, GL_QUERY>
{
private:
    GLuint mObjectName;

public:
    /// returns the raw object name to be used directly in OpenGL functions
    GLuint getObjectName() const { return mObjectName; }

public:
    /// saves the current timestamp
    /// result can be queried via isAvailable and get
    void save();

    /// returns true if the result can be queried without blocking the pipeline
    bool isAvailable();

    /// returns the saved timestamp (will block if !isAvailable)
    /// (time is in ns)
    GLuint64 getNanoseconds();
    /// same as getNanoseconds but converted to seconds
    double getSeconds();

public:
    Timestamp();
    ~Timestamp();

public:
    // creates a timestamp object
    static SharedTimestamp create();
};
}
