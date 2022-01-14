#pragma once
#include <core/time.hpp>
#include <glm/vec2.hpp>

namespace le::utils {
struct EngineStats {
	struct Counter;
	struct Frame {
		u64 count;
		u32 rate;
		Time_s dt;
	};
	struct Gfx {
		struct {
			u64 buffers;
			u64 images;
		} bytes;
		struct {
			glm::uvec2 swapchain;
			glm::uvec2 window;
			glm::uvec2 renderer;
		} extents;
		u32 drawCalls;
		u32 triCount;
	};

	Frame frame;
	Gfx gfx;
	Time_s upTime;
};

struct EngineStats::Counter {
	EngineStats stats{};
	struct {
		time::Point stamp{};
		Time_s elapsed{};
		u32 count{};
	} prev;

	void update() noexcept {
		++prev.count;
		++stats.frame.count;
		if (prev.stamp == time::Point()) {
			prev.stamp = time::now();
		} else {
			stats.frame.dt = time::diffExchg(prev.stamp);
			prev.elapsed += stats.frame.dt;
			stats.upTime += stats.frame.dt;
		}
		if (prev.elapsed >= 1s) {
			stats.frame.rate = std::exchange(prev.count, 0);
			prev.elapsed -= 1s;
		}
	}
};
} // namespace le::utils
