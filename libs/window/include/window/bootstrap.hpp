#pragma once
#include <window/android_instance.hpp>
#include <window/desktop_instance.hpp>

namespace le::window {
#if defined(LEVK_USE_GLFW)
using Instance = DesktopInstance;
#elif defined(__ANDROID__)
using Instance = AndroidInstance;
#else
static_assert(false, "Unsupported platform");
#endif
} // namespace le::window
