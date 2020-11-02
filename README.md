# Little Engine Vulkan

An attempt to write a simple 3D game engine with a (mostly hard-coded) Vulkan renderer.

[Documentation](https://karnkaul.github.io/levk-docs) is located here (WIP).

[![Build status](https://ci.appveyor.com/api/projects/status/km8h75k4a1695umo/branch/main?svg=true)](https://ci.appveyor.com/project/karnkaul/littleenginevk/branch/main)

### Features

- Multi-platform windowing
  - Multi-window support (Windows, Linux: via GLFW)
  - Windowing event / input callback registration
  - Keyboard, mouse, and gamepad support
- 3D renderer
  - Vulkan backend (internal)
  - Validation layer support (on by default in `Debug`)
  - Async buffer and image transfers
  - Phong shading (uber shader)
- Multi-threaded task system
  - Async queue (condition variable, no spinlock)
  - Configurable worker count
- Entity Component module
  - Hashed ID entities
  - Templated type-erased component storage (no polymorphism support)
  - Thread-safe registry
- Multi-threaded resourcing
  - Thread-safe resource management
  - Async resource loading
  - Hot reloading of textures and shaders
- JSON de/serialisation
- Free-look camera
- Editor overlay (via Dear ImGui)
- API Documentation (via Doxygen)

### Usage

- Clone this repo (manually initialising git submodules is not required, it will be done by CMake script)
- Use CMake and a generator of your choice

```
# Examples
cmake -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ . -B out/Debug
# OR
cmake . -B out/Debug
ccmake out/Debug
```

- Adjust CMake variables as needed: all variables pertaining to this project are prefixed with `LEVK`
- Build engine / demo / tests

```
cmake --build out/Debug
```

- Debug/Run desired executables / Link `levk-engine` to custom application

[Original repository](https://github.com/karnkaul/LittleEngineVk)

[LICENCE](LICENSE)
