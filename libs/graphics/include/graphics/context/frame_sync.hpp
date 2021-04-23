#pragma once
#include <functional>
#include <vector>
#include <core/not_null.hpp>
#include <graphics/context/command_buffer.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/render_types.hpp>
#include <graphics/utils/ring_buffer.hpp>

namespace le::graphics {
class Device;

struct FrameSync {
	struct Command {
		CommandBuffer commandBuffer;
		vk::CommandPool pool;
		vk::CommandBuffer buffer;
	};

	vk::Framebuffer framebuffer;
	RenderSync sync;
	Command primary;
	std::vector<Command> secondary;
	std::optional<u32> index;
};

class BufferedFrameSync : public RingBuffer<FrameSync> {
  public:
	BufferedFrameSync(not_null<Device*> device, std::size_t size, u32 secondaryCount = 0);
	BufferedFrameSync(BufferedFrameSync&&);
	BufferedFrameSync& operator=(BufferedFrameSync&&);
	~BufferedFrameSync();

	template <typename T>
	T& sync(RingBuffer<T>& out_source) const;

	void swap();
	void refreshSync();

  private:
	void destroy();

	not_null<Device*> m_device;
};

// impl

template <typename T>
T& BufferedFrameSync::sync(RingBuffer<T>& out_source) const {
	out_source.index = index;
	return out_source.get();
}
} // namespace le::graphics
