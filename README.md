# Little Engine

**A simple 3D game engine using C++20 and Vulkan.**

[![Build Status](https://github.com/karnkaul/LittleEngineVk/actions/workflows/ci.yml/badge.svg)](https://github.com/karnkaul/LittleEngineVk/actions/workflows/ci.yml)

https://github.com/karnkaul/LittleEngineVk/assets/16272243/304af3cc-de21-4900-95e7-5370d2197679

## Features

- [x] PBR materials
- [x] HDR lighting
- [x] GLTF mesh importer
- [x] Skinned meshes and animations
- [x] Fonts and text
- [x] Dynamic rendering (Vulkan)
- [x] In-game UI
- [x] Dear ImGui
- [x] AABB collisions
- [ ] Shadow mapping
- [ ] Audio

## Requirements

### Runtime

- OS with desktop environment and Vulkan loader (`libvulkan1.so` / `vulkan1.dll`)
  - Windows 10
  - Linux: X, Wayland (untested)
  - Mac OSX: untested, will require MoltenVk
- GPU supporting Vulkan 1.3+, its driver, and loader

### Development

- CMake 3.23+
- C++20 compiler and stdlib
- OS with desktop environment and Vulkan loader (`libvulkan1.so` / `vulkan1.dll`)
  - Windows 10
  - Linux: X, Wayland (untested)
  - Mac OSX: untested, will require MoltenVk
- GPU supporting Vulkan 1.3+, its driver, and loader
- `glslc` for compiling glsl shaders to SPIR-V
- (Optional) Vulkan SDK for validation layers (contains `glslc`)

## Usage

LittleEngine (`le::little-engine`) is a library intended to be built from source and linked statically, it currently does not support any installation / packaging. 
Link to it via CMake: `target_link_libraries(foo PRIVATE le::little-engine)`.

LittleEngine Scene (`le::le-scene`) is an optional wrapper that provides `Entity`, `Component`, `Scene`, `Runtime`, and other convenience facilities.

Default shaders are provided as GLSL sources in `shaders/`. (These can be compiled to SPIR-V into your data directory using LittleEngine's tools.)

See [example](example/example.cpp) for a basic example that renders an animated skinned mesh and some text, and responds to input.

### Building

- Clone this repository somewhere.
- Use CMake and a generator of your choice to configure an out-of-source build (`build` and `out` are ignored in git).
- If using CMake 3.20+ / Visual Studio in CMake mode / CMake Tools with VSCode, `CMakePresets.json` can be utilized (and/or extended via `CMakeUserPresets.json`) for convenience.
  - Use `cmake --preset <name>` to configure and `cmake --build --preset <name>` to build on the command line.
  - Visual Studio CMake and VS Code CMake Tools should pick up the presets by themselves.
- For other scenarios, use CMake GUI or the command line to configure and generate a build.
  - Command line: `cmake -S . -B out && cmake --build out`.
  - If using an IDE generator, use CMake GUI to configure and generate a build, then open the project/solution in the IDE and build/debug as usual.

### Tools

LittleEngine provides some basic tools separate from the engine for building data. There are two main aspects to building data:

**Shaders**

The engine expects shaders to be in SPIR-V binary format. Custom shaders can be used as long as the shader layouts are compatible, and custom descriptor sets are updated every frame, if any. The engine uses a single global pipeline layout and buffered descriptor sets per camera and per render object.

**Meshes**

The engine uses custom formats for materials, animations, skeletons, and meshes. GLTF support is built-in, and meshes from such assets can be imported using `le::importer`. Contributions for importing other formats are welcome.

Note: if your game / app only uses programmatically generated geometry, this step is not required as you won't be loading any mesh data. Images can be loaded as textures directly, the JSON that's generated by the importer is optional.

#### glsl2spirv

This tool uses `glslc` by default to compile GLSL shaders to SPIR-V. It takes the source and desitnation directories as optional arguments, and reproduces the source file tree at the destination, with each compiled shader being suffixed with `.spv`. This suffix is optional in `Uri`s in-game, the shader assets will automatically add them if not present.

If shaders are frequently modified, it is recommended to have a custom target in CMake that invokes `glsl2spirv`, to ensure a build run always uses up-to-date shaders.

#### le-importer

This tool imports GLTF meshes into LittleEngine meshes, geometries, materials, textures, animations, and skeletons.

## External Dependencies

- [GLFW](https://github.com/glfw/glfw)
- [glm](https://github.com/g-truc/glm)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [stb](https://github.com/nothings/stb)
- [VulkanHPP](https://github.com/KhronosGroup/Vulkan-Hpp)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [Freetype](https://github.com/freetype/freetype)
- [OpenAl Soft](https://github.com/kcat/openal-soft)
- [dr_libs](https://github.com/mackron/dr_libs)
- [Portable File Dialogs](https://github.com/samhocevar/portable-file-dialogs)
- [capo-lite](https://github.com/capo-devs/capo-lite)
- [clap](https://github.com/karnkaul/clap)
- [djson](https://github.com/karnkaul/djson)
- [dyvk](https://github.com/karnkaul/dyvk)
- [gltf2cpp](https://github.com/karnkaul/gltf2cpp)

## Misc

[Original repository](https://github.com/karnkaul/LittleEngineVk)

[LICENCE](LICENSE)
