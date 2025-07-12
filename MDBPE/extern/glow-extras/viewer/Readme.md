# Glow Viewer

Simple mesh viewer for debug and visualization.
Interacts well with polymesh.

## Usage

Minimal example:

```
pm::Mesh m;
auto pos = m.vertices().make_attribute<glm::vec3>();
load("path/to/file", m, pos);

view(pos);
```

## Generic `view` function

* Usage: `view(obj, args...)`
    * `obj` is the object you want to show
        * can be polymesh position attribute (`vertex_attribute<glm::vec3> const& pos`)
            * if mesh has faces same as `polygons(pos)`
            * otherwise, if mesh has edges same as `lines(pos)`
            * otherwise, same as `points(pos)`
        * can be `VertexArray`s
            * with some conventional naming
                * `aPosition`, `pos`, `position` for pos attribute
                * `aNormal`, `normal` for normal attribute
                * ...
            * with explicit naming
        * different built-in geometric types
            * `viewer::polygons(...)`
            * `viewer::triangles(...)`
            * `viewer::polylines(...)`
            * `viewer::lines(...)`
            * `viewer::points(...)`
            * `viewer::splats(...)`
            * `viewer::spheres(...)`
            * `viewer::planes(...)`
            * `viewer::splines(...)`
            * ...
        * can be built-in gizmos
            * `viewer::grid(...)`
            * `viewer::translate_handle(...)`
            * `viewer::rotate_handle(...)`
            * `viewer::scale_handle(...)`
            * `viewer::coordinate_frame(...)`
            * ...
        * can be other built-in types
            * `viewer::texts(...)`
            * `viewer::labels(...)`
            * `viewer::images(...)`
            * ...
        * some objects have further geometry properties
            * `.point_size(...)` for `viewer::points`
            * `.line_width(...)` for `viewer::lines`
            * `.normals(...)` for `viewer::triangles` (shortcuts: `.face_normals()` and `.vertex_normals()`)
            * ...
        * objects can be modified before being passed
            * `.transform(glm::mat4)`
            * `.move(glm::vec3)`
            * `.scale(glm::vec3)`
            * `.rotate(glm::vec3, float)`
            * `.rotateX`, `.rotateY`, `.rotateZ`
            * ...
        * can be a collection of objects
            * `std::vector<object>`
    * `args...` is a variadic list of setting up the renderable / view
        * this calls `view.configure(renderable, arg)` for each argument (and passes the created renderable)
        * `arg` can be a collection of arguments (`viewer::setup_args`)
        * affects all objects
        * can be material parameters
        * can be scene parameters
    * Materials can be configured (and passed as args)
        * `glow::colors::color` for setting a global color
        * `polymesh::XYZ_attribute<glow::colors::color>` for setting per-primitive colors
        * TODO: GGX model
        * TODO: custom shader
        * TODO: mapped data
        * ...
    * Scene can be configured (and passed as args)
        * TODO: background, skybox
        * TODO: lighting
    * Camera can be configured (and passed as args)
        * TODO: shared camera
        * TODO: look-at
        * TODO: different controllers
* Subviews
    * `SubView::show` attaches to the currently active subview or creates a window if root
    * RAII objects that `show` on exit
    * `Layout` classes that layout subviews

## Design / Structure

### Goals

* All common tasks should require as little code as possible
* Scenes should be accessible to create complex setups

### Features

* Multiple views
* Animations
* Interactions
* ImGui integration (imgui-in-viewer and viewer-in-imgui)
* Picking
* Render to file / texture / image
* Shared cameras
* Renderable types: mesh, wireframe, lines, points, splats, ...
* Material models: colors, textures, mapped data, GGX, IBL ...
* Shadowing fine control: caster, receiver
* Outline shader
* Gizmos
* Custom shaders

### Renderable

* A renderable is an object in the scene such as meshes, point clouds, gizmos.
* Geometry is independent from material model

### C++ Features

* Same functions are also inserted into the `polymesh` namespace so that for example `polygons(pos)` works without namespace
* Do not use `using namespace glow::viewer`. The library is designed to work ergonomically without that.

## TODO

* Interaction with `typed-geometry`
* Make raii classes attach properly