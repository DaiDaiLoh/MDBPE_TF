# Changelog

## Migration Guide

### to 1.0

* `typed-graphics` is now the required, internally used math library and `glm` is an optionally supported
* OpenGL 4.5+ is required

### to 0.9

* `glm` is an external dependency now (applications need to provide it themselves and make a target `glm` available)
* `ColorSpace` is now mandatory when loading textures

## Changes

### 1.0 (next version)

* Migration to C++17
* OpenGL 4.5+ is required

### 0.9 (current version)

* `glow::info()` now works with `glm` types again (and anything that either has `operator<<` or `to_string`)
* removed `fmt` and `snappy` dependencies
* made `glm` an externally-provided dependency
* CMake is now multi-config friendly (a single `.sln` for VS is sufficient now)
