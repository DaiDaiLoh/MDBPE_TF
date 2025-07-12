#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <clean-core/span.hh>

#include <glow/common/file_watch.hh>
#include <glow/common/non_copyable.hh>
#include <glow/common/shared.hh>

#include <glow/gl.hh>

#include "NamedObject.hh"

namespace glow
{
GLOW_SHARED(class, Shader);
GLOW_SHARED(class, ShaderParser);

class Shader final : public NamedObject<Shader, GL_SHADER>
{
    /// Global shader parser
    static SharedShaderParser sParser;

    /// OGL id
    GLuint mObjectName;

    /// Shader type
    GLenum mType;

    /// True iff shader has compile errors
    bool mHasErrors = false;

    /// True iff shader has a compiled version
    bool mCompiled = false;

    /// Filepath of this shader (if on-disk)
    std::string mFileName;
    /// File flag (if on-disk)
    FileWatch::SharedFlag mFileFlag;

    /// Dependency file flags
    std::vector<FileWatch::SharedFlag> mDependencyFlags;

    /// Primary source of the shader
    std::vector<std::string> mSources;

public: // getter
    GLuint getObjectName() const { return mObjectName; }
    GLenum getType() const { return mType; }
    bool isCompiled() const { return mCompiled; }
    bool hasErrors() const { return mHasErrors; }
    bool isCompiledWithoutErrors() const { return mCompiled && !mHasErrors; }
    /// Returns non-empty filename if created from file, otherwise ""
    std::string const& getFileName() const { return mFileName; }

public:
    Shader(GLenum shaderType);
    ~Shader();

    /// Compiles this shader
    /// Prints to the log should any error occur
    void compile();

    /// Fetches new source from disk (if backed by file)
    /// And recompiles
    void reload();

    /// Sets the shader source to a single string
    void setSource(std::string_view source);
    /// Sets the shader source to a list of strings
    void setSource(cc::span<std::string const> sources);
    void setSource(std::initializer_list<std::string> sources) { setSource(cc::span(sources.begin(), sources.size())); }

    /// Checks the file modification time and returns true if a newer version exists
    /// Returns false for non-file-backed shaders
    bool newerVersionAvailable();

    /// Sets a new shader parser
    /// nullptr is allowed
    static void setShaderParser(SharedShaderParser const& parser);

    /// Returns a non-empty path if the given path points can be
    static bool resolveFile(std::string_view name, GLenum& shaderType, std::string& content, std::string& realFileName);

private:
    /// Adds a file as dependency
    void addDependency(std::string_view filename);
    /// Signals a file change on disk
    void onDiskChange();

public: // static construction
    /// Creates and compiles (!) a shader from a given source string
    static SharedShader createFromSource(GLenum shaderType, std::string_view source);
    /// Creates and compiles (!) a shader from a given list of source strings
    static SharedShader createFromSource(GLenum shaderType, cc::span<std::string const> sources);
    static SharedShader createFromSource(GLenum shaderType, std::initializer_list<std::string> const& sources)
    {
        return createFromSource(shaderType, cc::span(sources.begin(), sources.size()));
    }
    /// Helper for embedded source files
    static SharedShader createFromSource(GLenum shaderType, const unsigned char source[]);
    /// Creates and compiles (!) a shader by loading the specified file
    static SharedShader createFromFile(GLenum shaderType, std::string_view filename);

    friend class Program;
    friend class ShaderParser;
};
}
