#pragma once
#include <le/core/enum_array.hpp>
#include <le/core/time.hpp>

namespace le {
struct FrameProfile {
	enum class Type {
		eAcquireFrame,
		eTick,
		eRenderShadowMap,
		eRenderScene,
		eRenderImGui,
		eRenderSubmit,

		eCOUNT_,
	};

	static constexpr auto to_string_v = StaticEnumToString<Type>{"acquire-frame", "tick", "render-shadow-map", "render-3d", "render-imgui", "render-submit"};

	EnumArray<Type, Duration> profile{};
	Duration frame_time{};
};
} // namespace le
