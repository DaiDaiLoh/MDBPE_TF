#include "uniform.hh"

#include <vector>

#include <glow/objects/Program.hh>
#include <glow/glow.hh>

namespace
{
thread_local std::vector<int> sBoolBuffer;
}

glow::detail::uniform_base::uniform_base(glow::UsedProgram& prog, std::string_view name, int size, GLenum type)
{
    if (!prog.isCurrent())
        return;

    checkValidGLOW();

    location = prog.program->useUniformLocationAndVerify(name, size, type);
}

void glow::detail::uniform<bool[]>::operator=(glow::array_view<const bool> v) const
{
    if (location == -1)
        return;

    if (sBoolBuffer.size() < v.size())
        sBoolBuffer.resize(v.size());

    auto const d = v.data();
    auto const s = int(v.size());

    for (auto i = 0; i < s; ++i)
        sBoolBuffer[i] = int(d[i]);

    glUniform1iv(location, int(v.size()), sBoolBuffer.data());
}
