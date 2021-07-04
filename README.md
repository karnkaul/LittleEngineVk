# Little Engine Vulkan

A simple C++20 3D game engine using a customizable Vulkan renderer.

[Documentation](https://karnkaul.github.io/levk-docs) is located here (WIP).

[![Build status](https://ci.appveyor.com/api/projects/status/pulw8g0clgeu58pm?svg=true)](https://ci.appveyor.com/project/karnkaul/littleenginevk) [![Codacy Badge](https://app.codacy.com/project/badge/Grade/e79ed5707cf34431aaba70cb2cd3779d)](https://www.codacy.com/gh/karnkaul/LittleEngineVk/dashboard?utm_source=github.com&utm_medium=referral&utm_content=karnkaul/LittleEngineVk&utm_campaign=Badge_Grade)

![Screenshot](demo/data/images/demo_screenshot_0.png)

### Features

- Multi-platform windowing
  - Windows, Linux: via GLFW
  - Android (alpha)
  - Keyboard, mouse, and gamepad support
- Bootstrapped Vulkan context
  - Customizable device selection
  - Dedicated transfer queue, async transfers
  - Shader reflection via SPIR-V Cross
  - Automatic descriptor Set management
  - Validation layer support (on by default in `Debug`)
- Asset Store
  - Store any `T` associated with a lightweight `io::Path` ID
  - Customizable asset loaders
  - Asset hot reload support
- Multi-threaded task scheduler
- Entity-Component framework
- JSON de/serialisation
- Customizable Editor

### Limitations

- No dynamic library support on Windows
- Single window instance
- Single draw command buffer

### Usage

- Clone this repo (manually initializing git submodules is not required, it will be done by the CMake script)
- Use CMake and a generator of your choice

```
# Examples
cmake -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ . -B out/db
# OR
cmake -S --preset=levk-db
```

- Adjust CMake variables as needed: all variables pertaining to this project are prefixed with `LEVK`
- Build engine / demo / tests

```
cmake --build out/db
```

- Debug/Run desired executables / Link `levk-engine` to custom application

### External Dependencies

- [{fmt}](https://github.com/fmtlib/fmt)
- [GLFW](https://github.com/glfw/glfw)
- [glm](https://github.com/g-truc/glm)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [PhysicsFS]()
- [stb](https://github.com/nothings/stb)
- [tiny-obj-loader](https://github.com/tinyobjloader/tinyobjloader)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)

[Original repository](https://github.com/karnkaul/LittleEngineVk)

[LICENCE](LICENSE)
