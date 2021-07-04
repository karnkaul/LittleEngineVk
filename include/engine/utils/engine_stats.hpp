#pragma once
#include <core/time.hpp>
#include <glm/vec2.hpp>

namespace le::utils {
struct EngineStats {
	struct Frame {
		Time_s ft;
		u32 rate;
		u64 count;
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
} // namespace le::utils
