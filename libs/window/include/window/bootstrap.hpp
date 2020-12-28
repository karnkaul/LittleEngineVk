#pragma once
#include <window/android_instance.hpp>
#include <window/desktop_instance.hpp>

namespace le::window {
#if defined(LEVK_DESKTOP)
using Instance = DesktopInstance;
#elif defined(LEVK_ANDROID)
using Instance = AndroidInstance;
#else
static_assert(false, "Unsupported platform");
#endif
} // namespace le::window
