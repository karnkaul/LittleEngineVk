#pragma once
#include <le/core/time.hpp>
#include <cstdint>
#include <string>

namespace le {
struct Stats {
	std::string gpu_name{};
	bool validation_enabled{};

	struct {
		std::uint64_t count{};
		std::uint64_t draw_calls{};
		Duration time{};
		std::uint32_t rate{};
	} frame{};

	struct {
		std::uint32_t shaders{};
		std::uint32_t pipelines{};
		std::uint32_t vertex_buffers{};
	} cache{};
};
} // namespace le
