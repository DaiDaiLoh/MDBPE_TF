# Changelog

## v0.7

First tagged version

### Changes

- Rewrote glow-extras-pipeline entirely
    - See main Readme for a brief introduction / feature list
    - Usage example: [Pipeline Sample](https://graphics.rwth-aachen.de:9000/Glow/glow-samples/tree/master/samples/wip/rendering-pipeline)
- Added Shader embedding
    - Global CMake option `GLOW_EXTRAS_EMBED_SHADERS` to enable / disable
    - Generates .hh and .cc file with an array of string pairs from a supplied folder of shader files
    - The project using embedded shaders registers each string pair with the `DefaultShaderParser`
    - Usage of embedded shaders does not differ from the usage of real, on-disk shaders, however embedded shaders do not allow hot-reloading (Re-run CMake and recompile required)
    - For an example of the registration, see `material::IBL::GlobalInit()` in glow-extras-material
    - For an example of the embed file generation, see the `CMakeLists.txt` of glow-extras-material (Line 5ff)
- Added `DebugOverlay` in glow-extras-debugging
    - On-screen ImGui OpenGL error log
    - Replaces default glow terminal output to make logging more readable for other things like Shader compilation
    - Integrated into `GlfwApp`, on by default
- Added new Camera
    - Replaced old cameras, which have been deprecated and moved to `legacy/`
- Added glow-extras-input
    - Extracts input handling logic previously tangled up in GlfwApp
    - WIP

### Update guide

There were a few breaking changes in this update:
- **GlfwApp**
    - `run()` no longer takes any arguments and no longer returns 0
    - `getCamera()` now returns a `SmoothedCamera` (instead of the previous `GenericCamera`)
    - `isKeyPressed()` and other input query methods have been deprecated, use `.input().isKeyHeld()` instead
- **glow-extras-camera**
    - `GenericCamera` and `FixedCamera` have been deprecated and moved to `legacy/`
    - The new counterpart of `GenericCamera` is `Camera` / `SmoothedCamera`, which are not as fully featured as `GenericCamera` (If you used features that are now missing, the old cameras are still there)
- **DebugRenderer**
    - The debug renderer has been decoupled from the old pipeline and features a simpler interface.
    - `render()` now only takes the VP-Matrix (Proj * View) as its argument
    - A new static `GlobalInit()` is required to be run before first use
    - Colors are now all `vec3`, down from `vec4`. Transparency is no longer supported.
- **glow-extras-pipeline** is a new library entirely. All glow-samples that used the old pipeline have been ported. Refer to the [Pipeline Sample](https://graphics.rwth-aachen.de:9000/Glow/glow-samples/tree/master/samples/wip/rendering-pipeline) when porting to the new pipeline.