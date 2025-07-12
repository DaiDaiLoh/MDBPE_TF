#pragma once

#include "ShaderParser.hh"

#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>

namespace glow
{
/// The default shader parser has the following extra functionality:
/// * resolve #include "filename" (include locally, if not found from include path)
/// * resolve #include <filename> (include from include paths)
/// * resolve custom registered #pragma keywords
/// * includes have an implicit #pragma once
/// * opengl #version is inserted automatically
class DefaultShaderParser : public ShaderParser
{
public:
    /// Given to PragmaCallbacks for them to output glsl code
    struct CallbackShaderWriter
    {
    private:
        std::stringstream& mStream;

        CallbackShaderWriter(std::stringstream& stream);

        friend class DefaultShaderParser;

    public:
        void emitText(std::string const& content);
        // void emitTextOnce(std::string const& key, std::string const& content); // TODO
        // void emitFile(std::string const& path); // TODO
    };

    /// Callback that is stored when a custom #glow instruction is registered
    /// Receives the instruction (#glow <registered keyword> <instruction>) and a reference to a CallbackShaderWriter
    /// Should return true if the instruction is recognized / valid, and false otherwise
    using PragmaCallback = std::function<bool(std::string const&, CallbackShaderWriter&)>;

private:
    /// default include paths (default is "./")
    static std::vector<std::string> sIncludePaths;

    /// explicitly includable shaders
    static std::unordered_map<std::string, std::string> sIncludeResources;

    static std::unordered_map<std::string, std::string> sVirtualFiles;

    /// callbacks to retrieve information about custom #pragmas
    static std::unordered_map<std::string, PragmaCallback> sPragmaCallbacks;

    void parseWithInclude(Shader* shader,
                          std::stringstream& parsedSrc,
                          std::string const& source,
                          int sourceIdx,
                          int& nextSrcIdx,
                          std::set<std::string>& includes,
                          std::string const& relativePath,
                          bool isVirtualFile = false);

    /// Attempts to resolve a filename include
    /// Returns "" if not found
    std::string resolve(std::string const& filename, std::string const& relPath);

public:
    DefaultShaderParser();

    /// sets all include paths (without trailing / )
    static void setIncludePaths(std::vector<std::string> const& paths);
    /// adds another include path (without trailing / )
    static void addIncludePath(std::string const& path);

    /// explicitly adds a file with content to the include structure
    static void addIncludeResource(std::string const& file, std::string const& content);

    /// adds a virtual file that will be correctly included with an #include <path> line
    static void addVirtualFile(std::string const& path, std::string const& content);
    static void addVirtualFile(std::string const& path, const unsigned char content[]);

    /// registers a custom #glow pragma keyword
    /// Format: #glow <keyword> <instruction>
    /// The provided PragmaCallback receives <instruction> and has to emit GLSL accordingly
    static void registerCustomPragma(std::string const& keyword, PragmaCallback const& callback);

    /// returns all registered include paths
    static std::vector<std::string> getIncludePaths();
    /// returns all explicitly registered "virtual files"
    static std::vector<std::string> getIncludeResources();

    std::vector<std::string> parse(Shader* shader, std::vector<std::string> const& sources) override;
    bool resolveFile(std::string_view name, GLenum& shaderType, std::string& content, std::string& realFileName) override;
};
}
