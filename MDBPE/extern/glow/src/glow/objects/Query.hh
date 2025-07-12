#pragma once

#include <glow/common/shared.hh>
#include <glow/gl.hh>

#include "NamedObject.hh"

namespace glow
{
GLOW_SHARED(class, Query);

/**
 * A generic OpenGL asynchronous query, target types can be:
 * SAMPLES_PASSED
 * ANY_SAMPLES_PASSED
 * PRIMITIVES_GENERATED
 * TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
 * TIME_ELAPSED
 *
 * See specialized queries below.
 *
 * Note: * Indexed queries are not jet supported.
 *       * Only one query per type is alowed to be active at any time.
 *       * Before the result can get read out, the query must end() !
 */
class Query : public NamedObject<Query, GL_QUERY>
{
protected:
    GLuint mObjectName;
    GLenum mTarget;

public:
    /// returns the raw object name to be used directly in OpenGL functions
    GLuint getObjectName() const { return mObjectName; }

public:
    Query(GLenum _defaultTarget);
    virtual ~Query() { glDeleteQueries(1, &mObjectName); }

public:
    /// start the query, only one query per type is allowed to be active at any time.
    void begin() { glBeginQuery(mTarget, mObjectName); }
    /// end the query
    void end() { glEndQuery(mTarget); }

    /// returns true if the result of the query is available, if not, trying to get the result will stall the CPU
    bool isResultAvailable()
    {
        GLuint resultAvailable;
        glGetQueryObjectuiv(mObjectName, GL_QUERY_RESULT_AVAILABLE, &resultAvailable);
        return resultAvailable == GL_TRUE;
    }

    /// get the query result, what it is depends on the query target
    GLuint getResult()
    {
        GLuint queryResult;
        glGetQueryObjectuiv(mObjectName, GL_QUERY_RESULT, &queryResult);
        return queryResult;
    }


    /// get the query result in 64 bit, what it is depends on the query target
    GLuint64 getResult64()
    {
        GLuint64 queryResult;
        glGetQueryObjectui64v(mObjectName, GL_QUERY_RESULT, &queryResult);
        return queryResult;
    }
};
}
