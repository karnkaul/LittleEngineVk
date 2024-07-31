#pragma once
#include <levk/core/time.hpp>
#include <cstdint>

namespace levk {
struct RenderStats {
	Seconds render_time{};
	std::int64_t draw_calls{};
	std::int64_t triangles{};
	std::int64_t pipelines{};
	std::int64_t pipeline_binds{};

	constexpr auto operator+=(RenderStats const& rhs) -> RenderStats& {
		render_time += rhs.render_time;
		draw_calls += rhs.draw_calls;
		triangles += rhs.triangles;
		pipelines += rhs.pipelines;
		pipeline_binds += rhs.pipeline_binds;
		return *this;
	}
};

constexpr auto operator+(RenderStats const& a, RenderStats const& b) -> RenderStats {
	auto ret = a;
	ret += b;
	return ret;
}
} // namespace levk
