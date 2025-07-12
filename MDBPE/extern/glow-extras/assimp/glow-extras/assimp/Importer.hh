#pragma once

#include <glow/common/non_copyable.hh>
#include <glow/common/property.hh>

#include <glow/fwd.hh>

#include <string>

#include "MeshData.hh"

namespace glow
{
namespace assimp
{
/**
 * @brief Importer for geometry
 *
 * Mesh attributes (some are optional/controlled by flags)
 *
 * vec3 aPosition; (always)
 * vec3 aNormal;
 * vec3 aTangent;
 * vec2 aTexCoord;
 * vec2 aTexCoord2; (3, 4, ... depending on mesh)
 * vec4 aColor;
 * vec4 aColor2; (3, 4, ... depending on mesh)
 *
 * TODO: Support for point clouds and lines
 * TODO: bones/animations
 *
 * Usage:
 *   // all-in-one:
 *   auto va = assimp::Importer().load(filename);
 *
 *   // thread-safe loadData separated from OpenGL access
 *   auto mesh = assimp::Importer().loadData(filename);
 *   auto va = mesh->createVertexArray();
 *
 *   (if you want to customize flags, the Importer can be saved locally)
 */
class Importer final
{
    GLOW_NON_COPYABLE(Importer);

private: // settings
    // see
    // http://www.assimp.org/lib_html/postprocess_8h.html#a64795260b95f5a4b3f3dc1be4f52e410a8857a0e30688127a82c7b8939958c6dc
    /// vec3 aTangent is available
    bool mCalculateTangents = true;
    /// No quads.
    bool mTriangulate = true;
    /// vec3 aNormal is available
    /// (normals in the dataset have priority)
    bool mGenerateSmoothNormal = true;
    /// Collapses scene structures
    bool mPreTransformVertices = true;
    /// vec2 aTexCoord is available (hopefully)
    bool mGenerateUVCoords = true;
    /// flips v coordinate if true
    bool mFlipUVs = false;

public:
    GLOW_PROPERTY(CalculateTangents);
    GLOW_PROPERTY(Triangulate);
    GLOW_PROPERTY(GenerateSmoothNormal);
    GLOW_PROPERTY(PreTransformVertices);
    GLOW_PROPERTY(GenerateUVCoords);
    GLOW_PROPERTY(FlipUVs);

public:
    Importer();

    /// Loads a VA from a given filename
    /// Supported file formats at time of writing this are:
    ///
    /// COMMON INTERCHANGE FORMATS
    ///   Autodesk ( .fbx )
    ///   Collada ( .dae )
    ///   glTF ( .gltf, .glb )
    ///   Blender 3D ( .blend )
    ///   3ds Max 3DS ( .3ds )
    ///   3ds Max ASE ( .ase )
    ///   Wavefront Object ( .obj )
    ///   Industry Foundation Classes (IFC/Step) ( .ifc )
    ///   XGL ( .xgl,.zgl )
    ///   Stanford Polygon Library ( .ply )
    ///   *AutoCAD DXF ( .dxf )
    ///   LightWave ( .lwo )
    ///   LightWave Scene ( .lws )
    ///   Modo ( .lxo )
    ///   Stereolithography ( .stl )
    ///   DirectX X ( .x )
    ///   AC3D ( .ac )
    ///   Milkshape 3D ( .ms3d )
    ///   * TrueSpace ( .cob,.scn )
    ///
    /// MOTION CAPTURE FORMATS
    ///   Biovision BVH ( .bvh )
    ///   * CharacterStudio Motion ( .csm )
    ///   GRAPHICS ENGINE FORMATS
    ///   Ogre XML ( .xml )
    ///   Irrlicht Mesh ( .irrmesh )
    ///   * Irrlicht Scene ( .irr )
    ///   GAME FILE FORMATS
    ///   Quake I ( .mdl )
    ///   Quake II ( .md2 )
    ///   Quake III Mesh ( .md3 )
    ///   Quake III Map/BSP ( .pk3 )
    ///   * Return to Castle Wolfenstein ( .mdc )
    ///   Doom 3 ( .md5* )
    ///   *Valve Model ( .smd,.vta )
    ///   *Open Game Engine Exchange ( .ogex )
    ///   *Unreal ( .3d )
    ///
    /// OTHER FILE FORMATS
    ///   BlitzBasic 3D ( .b3d )
    ///   Quick3D ( .q3d,.q3s )
    ///   Neutral File Format ( .nff )
    ///   Sense8 WorldToolKit ( .nff )
    ///   Object File Format ( .off )
    ///   PovRAY Raw ( .raw )
    ///   Terragen Terrain ( .ter )
    ///   3D GameStudio (3DGS) ( .mdl )
    ///   3D GameStudio (3DGS) Terrain ( .hmp )
    ///   Izware Nendo ( .ndo )
    ///
    /// Each attribute gets their own buffer.
    SharedVertexArray load(std::string const& filename) const;

    /// Load without creating a VertexArray
    SharedMeshData loadData(std::string const& filename) const;
};
}
}
