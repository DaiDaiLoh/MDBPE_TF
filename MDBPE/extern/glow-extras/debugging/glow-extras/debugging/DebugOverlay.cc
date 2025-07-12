#ifdef GLOW_EXTRAS_HAS_IMGUI

#include "DebugOverlay.hh"

#include <typed-geometry/feature/assert.hh>

#include <sstream>

#include <glow/glow.hh>
#include <glow/debug.hh>

namespace
{
void APIENTRY OverrideDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* message, const void*)
{
    // filter too verbose messages
    switch (id)
    {
    // NVidia
    case 131185: // Buffer detailed info 'will use VIDEO memory as the source for buffer object'
    case 131218: // Program/shader state performance warning: Fragment Shader is going to be recompiled because the
                 // shader key based on GL state mismatches.
    case 131186: // Buffer performance warning: Buffer object (bound to GL_SHADER_STORAGE_BUFFER, and
                 // GL_SHADER_STORAGE_BUFFER (0), usage hint is GL_STATIC_DRAW) is being copied/moved from VIDEO memory
                 // to HOST memory.
        return;
    }

    // ignore push/pop
    if (source == GL_DEBUG_SOURCE_APPLICATION && type == GL_DEBUG_TYPE_PUSH_GROUP)
        return;
    if (source == GL_DEBUG_SOURCE_APPLICATION && type == GL_DEBUG_TYPE_POP_GROUP)
        return;

    const char* sSource;
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        sSource = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        sSource = "Window System";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        sSource = "Shader Compiler";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        sSource = "3rd Party";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        sSource = "Application";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        sSource = "Other";
        break;
    default:
        sSource = "Unknown";
        break;
    }

    const char* sSeverity;
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        sSeverity = "High";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        sSeverity = "Medium";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        sSeverity = "Low";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        sSeverity = "Notification";
        break;
    default:
        sSeverity = "Unknown";
        break;
    }

    const char* sType;
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        sType = "Error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        sType = "Deprecated";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        sType = "Undefined";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        sType = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        sType = "Performance";
        break;
    case GL_DEBUG_TYPE_MARKER:
        sType = "Marker";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        sType = "Push";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        sType = "Pop";
        break;
    case GL_DEBUG_TYPE_OTHER:
        sType = "Other";
        break;
    default:
        sType = "Unknown";
        break;
    }

    std::stringstream ss;
    ss << "[" << sType << "][" << sSeverity << "][" << sSource << "][" << id << "] " << message << std::endl;
    glow::debugging::DebugOverlay::PrintErrorLine(ss.str().c_str());
}
}

namespace glow
{
namespace debugging
{
bool DebugOverlay::mInitialized = false;
bool DebugOverlay::mVisible = false;
bool DebugOverlay::mSuppressed = false;

ImGuiTextBuffer DebugOverlay::mImguiBuffer;
ImGuiTextFilter DebugOverlay::mImguiFilter;
ImVector<int> DebugOverlay::mImguiLineOffsets;
bool DebugOverlay::mScrollToBottom = false;

void DebugOverlay::PrintErrorLine(const char* content)
{
    if (!mVisible && !mSuppressed)
        ToggleVisibility();

    auto oldSize = mImguiBuffer.size();
    mImguiBuffer.appendf("%s", content);

    for (auto newSize = mImguiBuffer.size(); oldSize < newSize; ++oldSize)
        if (mImguiBuffer[oldSize] == '\n')
            mImguiLineOffsets.push_back(oldSize + 1);

    mScrollToBottom = true;
}

void DebugOverlay::ToggleVisibility()
{
    mVisible = !mVisible;

    if (mVisible)
        mSuppressed = false;
}

void DebugOverlay::Supress() { mSuppressed = true; }

void DebugOverlay::OnGui()
{
    TG_ASSERT(mInitialized);

    if (!mVisible)
        return;

    ImGui::SetNextWindowSize(ImVec2(850, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("OpenGL Debug Log");

    {
        if (ImGui::Button("Clear"))
            ClearErrorLog();

        ImGui::SameLine();

        if (ImGui::Button("Hide"))
            mVisible = false;

        ImGui::SameLine();

        if (ImGui::Button("Suppress"))
        {
            mVisible = false;
            mSuppressed = true;
        }

        ImGui::SameLine();
        mImguiFilter.Draw("Search", -100.f);
    }


    ImGui::Separator();

    {
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        auto bufBegin = mImguiBuffer.begin();
        auto bufEnd = mImguiBuffer.end();

        if (mImguiFilter.IsActive())
        {
            for (auto i = 0; i < mImguiLineOffsets.Size; ++i)
            {
                const char* line_start = bufBegin + mImguiLineOffsets[i];
                const char* line_end = (i + 1 < mImguiLineOffsets.Size) ? (bufBegin + mImguiLineOffsets[i + 1] - 1) : bufEnd;
                if (mImguiFilter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else
        {
            ImGuiListClipper clipper;
            clipper.Begin(mImguiLineOffsets.Size);
            while (clipper.Step())
            {
                for (auto i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                {
                    const char* line_start = bufBegin + mImguiLineOffsets[i];
                    const char* line_end = (i + 1 < mImguiLineOffsets.Size) ? (bufBegin + mImguiLineOffsets[i + 1] - 1) : bufEnd;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if (mScrollToBottom)
            ImGui::SetScrollHereY(1.0f);
        mScrollToBottom = false;

        ImGui::EndChild();
    }

    ImGui::End();
}

void DebugOverlay::ClearErrorLog()
{
    mImguiBuffer.clear();
    mImguiLineOffsets.clear();
    mImguiLineOffsets.push_back(0);
}

void DebugOverlay::Init()
{
    if (glow::OGLVersion.total >= 43)
    {
        // Override OpenGL Debug Callback
        glDebugMessageCallback(OverrideDebugMessageCallback, nullptr);
    }

    mInitialized = true;
}
}
}

#endif
