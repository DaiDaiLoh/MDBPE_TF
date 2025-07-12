# Glow Extras

Companion library for glow with convenience and helper functions. Separated into mostly independent sub-libraries.

## Libraries

### glfw

Efficiently get a Glow application up and running. Features:
- Context and window creation
- Input handling
- Main loop
- Basic profiling
- Optional default pipeline and camera setup

Noteworthy classes:
- **GlfwApp** - All-in-one Glow application
- **GlfwContext** - Used by GlfwApp, encapsulates a Window and its OpenGL context

Dependencies: [aion](https://graphics.rwth-aachen.de:9000/ptrettner/aion)

### pipeline

Pre-built, configurable clustered forward renderer. Read the **[guide](docs/Pipeline Guide.md)** for more. Features:
- Temporal Anti-Aliasing
- Tube Lights, GPU clustering
- Cascaded shadow mapping, PCF
- SSAO
- Order-independent transparency
- Bloom, Postprocessing

Noteworthy classes:
- **RenderPipeline** - Encapsulates every pipeline stage, used to interface with the pipeline
- **RenderScene** - POD-struct with all runtime-configurable pipeline settings

### camera

A camera, various controllers and several utility functions. Used by glfw and pipeline. Features:
- Camera matrix calculation
- WASD, mouselook and target orbit controllers
- Optional motion smoothing

Noteworthy classes:
- **Camera** - The camera
- **SmoothedCamera** - Like Camera but with smoothed movement, intercompatible

### geometry

Template-heavy library for generating simple geometry (VertexArrays in particular)

Noteworthy classes:
**Quad**, **Cube**, **UVSphere**, **FullscreenTriangle**

### assimp

Import mesh files to create VertexArrays.

Dependency: [assimp](https://www.graphics.rwth-aachen.de:9000/ptrettner/assimp-lean.git)

### timing

Timing and performance profiling.

Noteworthy classes:
- **CpuTimer** - High-precision, platform-independent CPU timer
- **GpuTimer** - High-precision, non-stalling GPU timer

Dependencies: [aion](https://graphics.rwth-aachen.de:9000/ptrettner/aion)

### Smaller libraries

- **colors** - Color utilities
- **debugging** - GUI OpenGL error log, line drawing helper
- **material** - PBR (GGX) shader helpers, IBL precalculation and utilities
- **input** - Input handling, used by GlfwApp
- **shader** - Miscellaneous shader helpers, WIP
- **viewer** - Polymesh Viewer, WIP

## Usage

Add git submodule
```
git submodule add https://www.graphics.rwth-aachen.de:9000/Glow/glow-extras.git
git submodule update --init --recursive
```

Add to cmake (be sure to do that _after_ glow)
```
add_subdirectory(path/to/glow-extras)
```

Choose which libraries you need

```
# all
target_link_libraries(YourLib PUBLIC glow-extras)

# .. or single ones
target_link_libraries(YourLib PUBLIC glow-extras-camera)
target_link_libraries(YourLib PUBLIC glow-extras-geometry)
target_link_libraries(YourLib PUBLIC glow-extras-pipeline)
```

**CAUTION**: If you use extras that require dependencies, you have to add them manually before.
E.g. for `glow-extras-assimp`:

```
add_subdirectory(path/to/assimp) # https://graphics.rwth-aachen.de:9000/ptrettner/assimp-lean
add_subdirectory(path/to/glow-extras) # AFTER dependencies

target_link_libraries(YourLib PUBLIC glow-extras-assimp) # internally depends on assimp
```
glow-extras will disable (and tell you) all sub-libs that were disabled due to missing dependencies.