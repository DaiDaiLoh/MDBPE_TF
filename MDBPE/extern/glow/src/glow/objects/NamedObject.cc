#include "NamedObject.hh"

#include <sstream>
#include <vector>

#include <glow/glow.hh>

std::string glow::detail::getObjectLabel(GLenum glNamespace, GLuint objectName)
{
    checkValidGLOW();

    // getObjectLabel requires OpenGL 4.3 or higher
    if (glow::OGLVersion.total < 43)
        return "";

    GLsizei len = 0;
    glGetObjectLabel(glNamespace, objectName, 0, &len, nullptr);

    std::vector<char> label;
    label.resize(unsigned(len + 1));
    glGetObjectLabel(glNamespace, objectName, len + 1, nullptr, label.data());

    return label.data(); // convert to string
}

void glow::detail::setObjectLabel(GLenum glNamespace, GLuint objectName, std::string_view label, size_t maxLength)
{
    checkValidGLOW();

    // glObjectLabel requires OpenGL 4.3 or higher
    if (glow::OGLVersion.total < 43)
        return;

    if (label.size() > maxLength)
    {
        auto s = std::string(label.substr(0, maxLength)) + "...";
        glObjectLabel(glNamespace, objectName, -1, s.c_str());
    }
    else
        glObjectLabel(glNamespace, objectName, -1, label.data());
}

std::string glow::detail::getNamedObjectString(GLenum glNamespace, GLuint objectName, std::string const& label)
{
    std::ostringstream oss;
    oss << "[";
    switch (glNamespace)
    {
    case GL_BUFFER:
        oss << "Buffer ";
        break;
    case GL_SHADER:
        oss << "Shader ";
        break;
    case GL_PROGRAM:
        oss << "Program ";
        break;
    case GL_VERTEX_ARRAY:
        oss << "VertexArray ";
        break;
    case GL_QUERY:
        oss << "Query ";
        break;
    case GL_PROGRAM_PIPELINE:
        oss << "ProgramPipeline ";
        break;
    case GL_TRANSFORM_FEEDBACK:
        oss << "TransformFeedback ";
        break;
    case GL_SAMPLER:
        oss << "Sampler ";
        break;
    case GL_TEXTURE:
        oss << "Texture ";
        break;
    case GL_RENDERBUFFER:
        oss << "Renderbuffer ";
        break;
    case GL_FRAMEBUFFER:
        oss << "Framebuffer ";
        break;
    default:
        oss << "UNKNOWN ";
        break;
    }
    oss << objectName;
    if (!label.empty())
        oss << ": " << label;
    oss << "]";
    return oss.str();
}
