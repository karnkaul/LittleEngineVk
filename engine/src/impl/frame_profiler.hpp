#pragma once
#include <le/core/wrap.hpp>
#include <le/frame_profile.hpp>
#include <le/graphics/buffering.hpp>
#include <array>
#include <optional>

namespace le {
struct FrameProfiler {
	using Type = FrameProfile::Type;

	Wrap<std::array<FrameProfile, graphics::buffering_v>> frame_profiles{};
	EnumArray<Type, Clock::time_point> start_map{};
	Clock::time_point frame_start{};
	std::optional<Type> previous_type{};

	void start() { frame_start = Clock::now(); }

	void profile(Type type) {
		start_map[type] = stop_previous();
		previous_type = type;
	}

	auto stop_previous() -> Clock::time_point {
		auto const ret = Clock::now();
		if (previous_type) { frame_profiles.get_current().profile[*previous_type] = ret - start_map[*previous_type]; } // NOLINT
		return ret;
	}

	void finish() {
		frame_profiles.get_current().frame_time = stop_previous() - frame_start;
		frame_profiles.advance();
		previous_type.reset();
	}

	[[nodiscard]] auto previous_profile() const -> FrameProfile const& { return frame_profiles.get_previous(); }

	static auto self() -> FrameProfiler& {
		static auto ret = FrameProfiler{};
		return ret;
	}
};
} // namespace le
