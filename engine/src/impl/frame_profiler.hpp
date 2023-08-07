#pragma once
#include <le/frame_profile.hpp>
#include <le/graphics/buffering.hpp>
#include <array>
#include <optional>

namespace le {
struct FrameProfiler {
	using Type = FrameProfile::Type;

	std::array<FrameProfile, graphics::buffering_v> frame_profiles{};
	EnumArray<Type, Clock::time_point> start_map{};
	Clock::time_point frame_start{};
	std::optional<Type> previous_type{};
	std::size_t current_index{};

	void start() { frame_start = Clock::now(); }

	void profile(Type type) {
		start_map[type] = stop_previous();
		previous_type = type;
	}

	auto stop_previous() -> Clock::time_point {
		auto const ret = Clock::now();
		if (previous_type) { frame_profiles[current_index].profile[*previous_type] = ret - start_map[*previous_type]; } // NOLINT
		return ret;
	}

	void finish() {
		frame_profiles[current_index].frame_time = stop_previous() - frame_start; // NOLINT
		current_index = (current_index + 1) % frame_profiles.size();
		previous_type.reset();
	}

	[[nodiscard]] auto previous_profile() const -> FrameProfile const& {
		auto const index = current_index == 0 ? frame_profiles.size() - 1 : current_index - 1;
		return frame_profiles[index]; // NOLINT
	}

	static auto self() -> FrameProfiler& {
		static auto ret = FrameProfiler{};
		return ret;
	}
};
} // namespace le
