# GLOW - OpenGL Object-oriented Wrapper

## Requirements

* C++17
* CMake 3.0+
* OpenGL 4.5+

## Dependencies

Most dependencies are configurable and can be replaced by own versions.

* `glm` (math, requires `GLM_ENABLE_EXPERIMENTAL` and `GLM_FORCE_CTOR_INIT` to be defined)

## Features and Goals

* Modern CMake Usage
* Modern C++ (C++17)
* Modern OpenGL (4.5+)
* OpenGL Object Binding via RAII
* Automatic location mapping negotiation
* Automatic texture unit assignment
* Easy-to-use Uniform Buffer
* ArrayBuffer setup via GLM types
* Built-in Shader/VAO/Texture loading using static functions
* Proper smart pointer usage
* Automatic resource reloading for debugging
* Lean core library with optional extensions
* Forward declarations to reduce compile time
* Doxygen documentation
