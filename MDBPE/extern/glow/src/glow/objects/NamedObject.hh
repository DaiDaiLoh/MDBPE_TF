#pragma once

#include <glow/common/non_copyable.hh>
#include <glow/gl.hh>

#include <string>
#include <string_view>

namespace glow
{
namespace detail
{
std::string getObjectLabel(GLenum glNamespace, GLuint objectName);
void setObjectLabel(GLenum glNamespace, GLuint objectName, std::string_view label, size_t maxLength);
std::string getNamedObjectString(GLenum glNamespace, GLuint objectName, std::string const& label);
}

template <typename T, GLenum glNamespace>
class NamedObject
{
    GLOW_NON_COPYABLE(NamedObject);

protected:
    NamedObject() = default;

public:
    // returns the OpenGL Object Label
    std::string getObjectLabel() const { return detail::getObjectLabel(glNamespace, static_cast<T const*>(this)->getObjectName()); }

    // sets the OpenGL Object Label
    void setObjectLabel(std::string_view label, size_t maxLength = 200)
    {
        detail::setObjectLabel(glNamespace, static_cast<T const*>(this)->getObjectName(), label, maxLength);
    }
};

template <typename T, GLenum glNamespace>
std::string to_string(NamedObject<T, glNamespace> const* obj)
{
    return detail::getNamedObjectString(glNamespace, static_cast<T const*>(obj)->getObjectName(), obj->getObjectLabel());
}
}
