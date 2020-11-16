#pragma once
#include <window/desktop_instance.hpp>
#include <window/instance.hpp>

namespace le::window {
struct TInstance final {
#if defined(LEVK_USE_GLFW)
	using type = DesktopInstance;
#else
	static_assert(false, "Unsupported platform");
#endif
};

using Instance_t = TInstance::type;

Instance_t instance(CreateInfo const& info);

// impl

inline Instance_t instance(CreateInfo const& info) {
	return Instance_t(info);
}
} // namespace le::window
