# Little Engine Vulkan

A simple C++20 3D game engine using a customizable Vulkan renderer.

[Documentation](https://karnkaul.github.io/levk-docs) is located here (WIP).

[![Build status](https://ci.appveyor.com/api/projects/status/pulw8g0clgeu58pm?svg=true)](https://ci.appveyor.com/project/karnkaul/littleenginevk) [![Codacy Badge](https://app.codacy.com/project/badge/Grade/e79ed5707cf34431aaba70cb2cd3779d)](https://www.codacy.com/gh/karnkaul/LittleEngineVk/dashboard?utm_source=github.com&utm_medium=referral&utm_content=karnkaul/LittleEngineVk&utm_campaign=Badge_Grade)

![Screenshot](demo/data/images/demo_screenshot_0.png)

### Features

- `window`
  - Multi-platform windowing
    - Windows, Linux: via GLFW
  - Keyboard, mouse, and gamepad support
- `graphics`
  - Bootstrapped Vulkan render context
  - Customizable off-screen (forward) renderer
  - Multiple command buffers per frame
  - Customizable device selection
  - Deferred resource release
  - Async transfers
  - Shader reflection via SPIR-V Cross
  - Automatic pipeline creation
  - Per pipeline descriptor set lists
  - Stall-less API (including swapchain recreation)
  - sRGB and linear colour spaces
  - TrueType font support via dynamic texture atlases
- `engine`
  - Thread-safe `AssetStore`
    - Multi-reader single-writer API
    - Store any `T` associated with a `Hash`
    - Customizable asset loaders
    - Asset hot reload support: shaders, pipelines, textures implemented
  - Customizable Editor (via Dear ImGui)
  - Bitmap Fonts and 2D Text
  - GUI view/widget trees
  - Basic 3D AABB collision detection
- `libs/*`
  - Multi-threaded task scheduler
  - Archetypal entity-component framework
  - JSON parser, de/serializer
  - Formatted logger

### Limitations

- No CMake install stage / `find_package` support (as yet)
- No dynamic library support on Windows
- Single render pass

### Requirements

- CMake 3.14+
- Git
- C++20 compiler and stdlib (modules, coroutines, and ranges are not yet used)
- OS with desktop environment and Vulkan loader (`libvulkan1.so` / `vulkan1.dll`)
  - Windows 10
  - Linux: X, Wayland (untested), Raspberry Pi OS (64 bit bullseye+)
- GPU supporting Vulkan 1.0+, its driver, and loader
- Vulkan SDK / `glslc` (for compiling glsl shaders to SPIR-V in Debug, and validation layers)

### Usage

LittleEngineVk (`levk-engine`) is a library intended to be built from source and linked statically, it currently does not support any installation / packaging. At present it ships with one hacked-together `levk-demo` application as an example, which will be split up and expanded upon in the future.

#### Building `levk-demo`

- Clone this repo (manually initializing git submodules is not required, it will be done by the CMake script)
- Use CMake and a generator of your choice to configure an out-of-source build (`build` and `out` are ignored in git)
- If using CMake 3.20+ / Visual Studio in CMake mode / CMake Tools with VSCode, `cmake/CMakePresets.json` can be copied/symlinked to the project root for convenience
  - Use `cmake --preset <name>` to configure and `cmake --build --preset <name>` to build on the command line
  - VS CMake and VSCode CMake Tools should pick up the presets by themselves
  - The presets are not checked into the repo root since some IDEs/editors force its usage if present there
- For other scenarios, use CMake GUI or the command line to configure and generate a build
  - Command line: `cmake -S . -B out`
  - If using an IDE generator, open the project/solution in the IDE and build/debug as usual
  - Build on the command line via `cmake --build out`

### External Dependencies

- [{fmt}](https://github.com/fmtlib/fmt)
- [GLFW](https://github.com/glfw/glfw)
- [glm](https://github.com/g-truc/glm)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [PhysicsFS](https://icculus.org/physfs/)
- [stb](https://github.com/nothings/stb)
- [tiny-obj-loader](https://github.com/tinyobjloader/tinyobjloader)
- [VulkanHPP](https://github.com/KhronosGroup/Vulkan-Hpp)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [SPIR-V Cross](https://github.com/KhronosGroup/SPIRV-Cross)
- [freetype](https://gitlab.freedesktop.org/freetype/freetype.git)

### CMake submodules

- [ktl](https://github.com/karnkaul/ktl)
- [dtest](https://github.com/karnkaul/dtest)

[Original repository](https://github.com/karnkaul/LittleEngineVk)

[LICENCE](LICENSE)
