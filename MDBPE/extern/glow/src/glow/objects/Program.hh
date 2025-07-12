#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <clean-core/span.hh>

#include <glow/common/gltypeinfo.hh>
#include <glow/common/nodiscard.hh>
#include <glow/common/non_copyable.hh>
#include <glow/common/property.hh>
#include <glow/common/shared.hh>
#include <glow/common/string_map.hh>

#include <glow/gl.hh>

#include <glow/util/LocationMapping.hh>

#include "NamedObject.hh"

#include "raii/UsedProgram.hh"

namespace glow
{
GLOW_SHARED(class, Sampler);
GLOW_SHARED(class, Shader);
GLOW_SHARED(class, Program);
GLOW_SHARED(class, LocationMapping);
GLOW_SHARED(class, UniformState);
GLOW_SHARED(class, Texture);
GLOW_SHARED(class, Texture2D);
GLOW_SHARED(class, UniformBuffer);
GLOW_SHARED(class, ShaderStorageBuffer);
GLOW_SHARED(class, AtomicCounterBuffer);
GLOW_SHARED(class, Buffer);

class Program final : public NamedObject<Program, GL_PROGRAM>
{
public:
    struct UniformInfo
    {
        std::string name;
        GLint location = -1;
        GLint size = -1;
        GLenum type = GL_INVALID_ENUM;
        bool wasSet = false;

        UniformInfo() = default;
        UniformInfo(std::string name, GLint location, GLint size, GLenum type)
          : name(std::move(name)), location(location), size(size), type(type), wasSet(false)
        {
        }
    };

private:
    /// If true, shader check for shader reloading periodically
    static bool sCheckShaderReloading;


    /// OpenGL object name
    GLuint mObjectName;

    /// List of attached shader
    std::vector<SharedShader> mShader;
    /// Last time of reload checking
    int64_t mLastReloadCheck = 0;
    /// Last time this shader was linked
    int64_t mLastTimeLinked = 0;

    /// Uniform lookup cache
    /// Invalidates after link
    mutable std::vector<UniformInfo> mUniformCache;

    /// Texture unit mapping
    LocationMapping mTextureUnitMapping;
    /// Bounds texture (idx = unit)
    std::vector<SharedTexture> mTextures;
    /// Bounds texture sampler (idx = unit)
    std::vector<SharedSampler> mSamplers;

    /// Locations for in-VS-attributes
    /// At any point, the mapping saved here must be consistent (i.e. a superset) of the GPU mapping
    SharedLocationMapping mAttributeMapping;
    /// Locations for out-FS-attributes
    /// At any point, the mapping saved here must be consistent (i.e. a superset) of the GPU mapping
    SharedLocationMapping mFragmentMapping;
    /// Friend for negotiating mAttributeMapping
    friend class VertexArray;
    friend struct BoundVertexArray;
    /// Friend for negotiating mFragmentMapping
    friend class Framebuffer;
    friend struct BoundFramebuffer;

    /// Mapping for uniform buffers
    LocationMapping mUniformBufferMapping;
    /// Bound uniform buffer
    util::string_map<SharedUniformBuffer> mUniformBuffers;

    /// Mapping for shaderStorage buffers
    LocationMapping mShaderStorageBufferMapping;
    /// Bound shaderStorage buffer
    util::string_map<SharedShaderStorageBuffer> mShaderStorageBuffers;

    /// Bound AtomicCounter buffer
    std::map<int, SharedAtomicCounterBuffer> mAtomicCounterBuffers;

    /// List of varyings for transform feedback
    std::vector<std::string> mTransformFeedbackVaryings;
    /// Buffer mode for transform feedback
    GLenum mTransformFeedbackMode;
    /// if true, this program is linked properly for transform feedback
    bool mIsLinkedForTransformFeedback = false;

    /// true if already checked for layout(loc)
    bool mCheckedForAttributeLocationLayout = false;
    /// true if already checked for layout(loc)
    bool mCheckedForFragmentLocationLayout = false;

    /// disables unchanged uniform warning
    bool mWarnOnUnchangedUniforms = true;
    /// true if already checked for unchanged uniforms
    bool mCheckedUnchangedUniforms = false;

private:
    /// Internal linking
    /// returns false on errors
    bool linkAndCheckErrors();

    /// Restores additional "program state", like textures and shader buffers
    void restoreExtendedState();

    /// Reset frag location check
    /// Necessary because frags cannot be queried ...
    void resetFragmentLocationCheck() { mCheckedForFragmentLocationLayout = false; }

    /// see public: version
    UniformInfo* getUniformInfo(std::string_view name);

    /// returns uniform location and sets "wasSet" to true
    /// Also verifies that the uniform types match
    GLint useUniformLocationAndVerify(std::string_view name, GLint size, GLenum type);

public: // properties
    GLuint getObjectName() const { return mObjectName; }
    std::vector<SharedShader> const& getShader() const { return mShader; }
    SharedLocationMapping const& getAttributeMapping() const { return mAttributeMapping; }
    SharedLocationMapping const& getFragmentMapping() const { return mFragmentMapping; }
    LocationMapping const& getTextureUnitMapping() const { return mTextureUnitMapping; }
    LocationMapping const& getShaderStorageBufferMapping() const { return mShaderStorageBufferMapping; }
    LocationMapping const& getUniformBufferMapping() const { return mUniformBufferMapping; }

    GLOW_GETTER(LastTimeLinked);

    GLOW_PROPERTY(WarnOnUnchangedUniforms);

    /// returns true iff a shader with this type is attached
    bool hasShaderType(GLenum type) const;

    /// Gets the currently used program (nullptr if none)
    static UsedProgram* getCurrentProgram();

    /// Modifies shader reloading state
    static void setShaderReloading(bool enabled);

public:
    /// Checks if all bound textures have valid mipmaps
    void validateTextureMipmaps() const;

    /// if not already done so:
    /// checks if any uniform is used but not set
    void checkUnchangedUniforms();

public: // gl functions without use
    /// Returns the in-shader location of the given uniform name
    /// Is -1 if not found or optimized out (!)
    /// Uses an internal cache to speedup the call
    GLint getUniformLocation(std::string_view name) const;

    /// Returns information about a uniform
    /// If location is -1, information is nullptr
    /// (Returned pointer is invalidated if shader is relinked)
    UniformInfo const* getUniformInfo(std::string_view name) const;

    /// Returns the index of a uniform block
    GLuint getUniformBlockIndex(std::string_view name) const;

    // ========================================= UNIFORMS - START =========================================
    /// Getter for uniforms
    /// If you get linker errors, the type is not supported
    /// Returns default-constructed values for optimized uniforms
    /// Usage:
    ///    auto pos = prog->getUniform<tg::pos3>("uPosition");
    ///
    /// LIMITATIONS:
    /// currently doesn't work for arrays/structs
    ///
    /// Supported types:
    ///  * bool
    ///  * float
    ///  * int
    ///  * unsigned int
    ///  * [ uib]vec[234]
    ///  * mat[234]
    ///  * mat[234]x[234]

    template <typename DataT>
    DataT getUniform(std::string_view name) const
    {
        DataT value;
        auto loc = getUniformLocation(name);
        if (loc >= 0)
            implGetUniform(glTypeOf<DataT>::basetype, loc, reinterpret_cast<void*>(&value));
        else
            value = DataT{};
        return value;
    }

    // ========================================== UNIFORMS - END ==========================================

private:
    /// Internal generic getter for uniforms
    void implGetUniform(detail::glBaseType type, GLint loc, void* data) const;

public:
    Program();
    ~Program();

    /// Returns true iff program is linked properly
    bool isLinked() const;

    /// Returns a list of all attribute locations
    /// Depending on driver, this only returns "used" attributes
    std::vector<std::pair<std::string, int>> extractAttributeLocations();

    /// Attaches the given shader to this program
    /// Requires re-linking!
    void attachShader(SharedShader const& shader);

    /// Links all attached shader into the program.
    /// Has to be done each time shader and/or IO locations are changed
    /// CAUTION: linking deletes all currently set uniform values!
    void link(bool saveUniformState = true);

    /// Configures this shader for use with transform feedback
    /// NOTE: cannot be done while shader is in use
    void configureTransformFeedback(cc::span<std::string const> varyings, GLenum bufferMode = GL_INTERLEAVED_ATTRIBS);
    void configureTransformFeedback(std::initializer_list<std::string> varyings, GLenum bufferMode = GL_INTERLEAVED_ATTRIBS)
    {
        configureTransformFeedback(cc::span(varyings.begin(), varyings.size()), bufferMode);
    }
    /// Returns true if transform feedback can be used
    bool isConfiguredForTransformFeedback() const { return mTransformFeedbackVaryings.size() > 0; }
    /// Extracts all active uniforms and saves them into a UniformState object
    /// This function should not be used very frequently
    SharedUniformState getUniforms() const;

    /// Binds a uniform buffer to a given block name
    /// DOES NOT REQUIRE PROGRAM USE
    void setUniformBuffer(std::string_view bufferName, SharedUniformBuffer const& buffer);
    /// Binds a shader storage buffer to a given block name
    /// DOES NOT REQUIRE PROGRAM USE
    void setShaderStorageBuffer(std::string_view bufferName, SharedShaderStorageBuffer const& buffer);
    /// Binds an atomic counter buffer to a binding point
    /// DOES NOT REQUIRE PROGRAM USE
    void setAtomicCounterBuffer(int bindingPoint, SharedAtomicCounterBuffer const& buffer);

    /// Verifies registered offsets in the uniform buffer and emits errors if they don't match
    /// Return false if verification failed
    bool verifyUniformBuffer(std::string_view bufferName, SharedUniformBuffer const& buffer);

    /// Activates this shader program.
    /// Deactivation is done when the returned object runs out of scope.
    GLOW_NODISCARD UsedProgram use() { return {this}; }
    friend UsedProgram;
    friend detail::uniform_base;

public: // static construction
    /// Creates a shader program from a list of shaders
    /// Attaches shader and links the program
    /// Program can be used directly after this
    static SharedProgram create(cc::span<SharedShader const> shader);
    static SharedProgram create(std::initializer_list<SharedShader> shader) { return create(cc::span(shader.begin(), shader.size())); }
    static SharedProgram create(SharedShader const& shader) { return create(cc::span(shader)); }
    /// Creates a program from either a single file or auto-discovered shader files
    /// E.g. createFromFile("mesh");
    ///   will use mesh.vsh as vertex shader and mesh.fsh as fragment shader if available
    /// See common/shader_endings.cc for a list of available endings
    /// Will also traverse "dots" if file not found to check for base versions
    /// E.g. createFromFile("mesh.opaque");
    ///   might find mesh.opaque.fsh and mesh.vsh
    static SharedProgram createFromFile(std::string_view fileOrBaseName);
    /// Creates a program from a list of explicitly named files
    /// Shader type is determined by common/shader_endings.cc
    static SharedProgram createFromFiles(std::vector<std::string> const& filenames);
};
}
