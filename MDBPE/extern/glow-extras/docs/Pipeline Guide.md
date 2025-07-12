# Pipeline guide

## Quickstart

1. Derive from `GlfwApp`
2. Call `setUsePipeline(true)` before `GlfwApp::init()`
3. Override the passes from `pipeline::RenderCallback` you need
```C++
void onRenderOpaquePass(glow::pipeline::RenderContext const& ctx) override;
```
4. Use the provided `RenderContext` to prepare your shader
```C++
void MyApp::onRenderOpaquePass(glow::pipeline::RenderContext const& ctx) {
    auto usedShader = ctx.useProgram(mMyShader);
    
    usedShader.setUniform("uView", ctx.camData.view);
    usedShader.setUniform("uProj", ctx.camData.proj);
    
    mesh->bind().draw();
}
```
Depending on the pass you implement, the shader requires special content. Read on, or refer to the comments in `glow-extras/pipeline/RenderCallback.hh` for more information.

## Render passes

The following is a list of passes a user can implement, ordered by importance. All of them are optional, and most require special additions to used shaders.

### Opaque pass

The meat of the pipeline. Draw fully opaque geometry that can be lit and shadowed using simple shader helpers.
A fragment shader that operates in this pass must not have any out declarations, and has to contain these lines at the start:
```C
#glow pipeline opaque
#include <glow-pipeline/pass/opaque/opaquePass.glsl>
```
It must output geometry like this:
```GLSL
outputOpaqueGeometry(color, getFragmentVelocity(positionHdc, prevPositionHdc));
```
The second argument is the fragment velocity, used for temporal anti-aliasing. It is optional, but the visuals will greatly suffer under missing velocities. `positionHdc` and `prevPositionHdc` are the vertex positions in homogenous device coordinates, and preferrably calculated in the vertex shader like this:
```GLSL
vsPositionHdc = uCleanVP * uModel * vec4(aPosition, 1);
vsPrevPositionHdc = uPrevCleanVP * uPrevModel * vec4(aPosition, 1);
```
with `uCleanVP` being `ctx.camData.cleanVp`, and `uPrevCleanVP` being `ctx.camData.prevCleanVP` (in the `RenderContext const& ctx` argument). `uPrevModel` is the model matrix from the previous rendered frame. So, if your meshes move, you have to store their previous model matrices in order to calculate velocities correctly.

#### Shader features

**Ambient Occlusion**
```GLSL
vec3 applyAo(in vec3 color, in float materialAo = 1.f);
```
Apply ambient occlusion to the color. This combines SSAO (previously computed by the pipeline internally) and AO as a material parameter, which defaults to 1 (fully lit).

**Shadowing**
```GLSL
float getShadowVisibility(vec3 worldSpacePosition);
```
Get the directional shadow amount at the fragment, in a range of 0 to 1. Usually applied to sunlight only, for example `color += getShadowVisibility(worldPos) * sunColor`.

**Lights**

By the nature of a clustered renderer, each fragment is assigned a set of light sources that affect it. These can be accessed with a special macro in the shader, like this:
```GLSL
FOREACH_LIGHT(i)
{
    LightData light = getLight(i);
}
```
`LightData` is a struct with all information concerning the light source. From it, the light incidence vector as well as the radiance can be extracted as such:
```GLSL
LightData light = getLight(i);

vec3 L;
vec3 radiance;
getTubeLightInfo(light, worldPos, V, N, roughness, L, radiance);

// Use L and radiance...
```
These values can then be used with any point light shading model. A good fit is the GGX implementation in `glow-extras-material`, as can be seen in the RenderingPipeline sample. Note that `roughness` is a required argument for the area normalization for sphere- and tube lights, matched for a GGX-like interpretation of roughness (from UE4's shading model).

### Transparent pass

Draw transparent geometry using weighted, blended order-independent transparency.
A fragment shader that operates in this pass must not have any out declarations, and has to contain these lines at the start:
```C
#glow pipeline transparent
#include <glow-pipeline/pass/transparent/oitPass.glsl>
```
It must output geometry like this:
```GLSL
outputOitGeometry(gl_FragCoord.z, color, alpha, offset);
```
The three latter arguments are not trivially available, and can be calculated utilizing helpers. Three structs exist for this purpose:
`OITFragmentInfo` - Contains information regarding the fragment's position and the camera. It has to be filled manually
`OITMaterial` - Contains information regarding the material. It has to be filled manually, except for `alpha`, which can be calculated using a helper.
`OITResult` - Contains the `color`, `alpha` and `offset`, can be calculated from the two other structs:
```GLSL
OITFragmentInfo fragInfo;
// Fill the struct...

OITMaterial material;
// Fill the struct...
material.alpha = getOitMaterialAlpha(fragInfo.N, fragInfo.V, material.specular) * uMaterialAlpha;

OITResult result;
getOitResult(material, fragInfo, result);

outputOitGeometry(gl_FragCoord.z, result.color, material.alpha, result.offset);
```

#### Transparency and TAA

Transparent objects have no velocity, but are still subject to temporal antialiasing (rendered with jittered projection matrix, reprojected). This can cause visual artifacts as the TAA history rejection struggles to make sense of the geometry.

### Depth-Pre pass

Pre-draw geometry that will be drawn in the opaque pass. Decreases overdraw, and SSAO is only cast by geometry drawn in this pass. It is entirely optional, and also optional per geometry (the geometry in the opaque pass can be a superset of the geometry here).
This pass **does not require a fragment shader**, but if you use one, it is recommended to include these two lines:
```C
#glow pipeline depthPre
#include <glow-pipeline/pass/depthpre/depthPre.glsl>
```
Then output geometry using
```GLSL
outputDepthPreGeometry();
```

### Shadow pass
Draw geometry that casts shadows.

#### Shadow pass - simple interface
If you have meshes that require no further processing (no vertex animations, no fragment discards etc.), a simple interface is provided which supplies its own internal shader to perform all the necessary transformations into light space, and geometry shader-based instancing to render into all cascades in one draw call.
Instead of the shadow pass method, use this one instead:
```C++
void onPerformShadowPass(RenderContext const& ctx, Program::UsedProgram& shader) override;
```
Then, for each mesh, upload its model matrix to the `"uModel"` uniform variable and draw it without any custom shader. The mesh must provide the `aPosition` attribute.
There is also such a helper for instanced rendering:
```C++
void onPerformShadowPassInstanced(RenderContext const& ctx, Program::UsedProgram& shader) override;
```
Use the provided shader and draw a vertex array with divisor 1. It must provide the `aPosition` attribute, as well as a column-divided per-instance model matrix in attributes `aModelC0`, ..., `aModelC3`. The exact specification can be gathered from `shader/glow-pipeline/internal/pass/shadow/geometryDepthOnlyInstanced.vsh`.


#### Shadow pass - custom shaders
If your shadow casters require a custom shader, override the full pass method instead, as with the other passes.
This pass requires either a vertex- or geometry shader of this form:
```GLSL
#include <glow-pipeline/pass/shadow/shadow.glsl>
// ...
outputShadowVertex(modelSpacePosition, cascadeIndex);
```
A fragment shader is optional, but can be added to discard fragments.

## Configuration

### RenderScene

All of the possible runtime pipeline configuration is encompassed by the `RenderScene` struct. It contains various parameters, some of them are: 
- Clear color
- Atmospheric scattering config
- Sun direction and color
- SSAO parameters
- Bloom cutoff and intensity
- Postprocessing settings
- Color grading LUT

### Lights

Since the pipeline uses a clustered forward renderer, a rather large number of lights can be evaluated at reasonable performance. All lights are assigned to the clusters of the camera frustum and made accessible to the fragment shaders of the opaque pass. The pipeline internally only uses tube lights, which degenerate to sphere- and point lights whenever needed.

To register lights, override this `RenderCallback` method:
```C++
void onGatherLights(LightCallback&) override;
```
It supplies a `LightCallback`, which is used to register any sort of light visible in the scene:
```C++
void onGatherLights(LightCallback& lc) {
    lc.addLight(position, color, radius); // Point Light
    lc.addLight(position, color, size, radius); // Sphere Light
    lc.addLight(positionA, positionB, color, size, radius); // Tube Light
}
```

## Miscellaneous

### 3D position query

Receive the 3D position of geometry (that is present in the depth buffer) located at a given pixel.
```C++
optional<glm::vec3> RenderPipeline::queryPosition3D(glm::uvec2 const &pixel) const
```
