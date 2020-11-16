#pragma once
#include <core/traits.hpp>
#include <graphics/context/defer_queue.hpp>
#include <graphics/context/memory.hpp>
#include <graphics/context/transfer.hpp>

namespace le::graphics {
class Device;
using LayoutTransition = std::pair<vk::ImageLayout, vk::ImageLayout>;

class VRAM final : public Memory {
  public:
	using Future = Transfer::Future;

	VRAM(Device& device, Transfer::CreateInfo const& transferInfo = {});
	~VRAM();

	View<Buffer> createBO(std::string_view name, vk::DeviceSize size, vk::BufferUsageFlags usage, bool bHostVisible);

	[[nodiscard]] Future copy(CView<Buffer> src, View<Buffer> dst, vk::DeviceSize size = 0);
	[[nodiscard]] Future stage(View<Buffer> deviceBuffer, void const* pData, vk::DeviceSize size = 0);
	[[nodiscard]] Future copy(Span<Span<u8>> pixelsArr, View<Image> dst, LayoutTransition layouts);

	void defer(View<Buffer> buffer, u64 defer = Deferred::defaultDefer);
	void defer(View<Image> image, u64 defer = Deferred::defaultDefer);

	template <typename Cont = std::initializer_list<Ref<Future>>>
	void wait(Cont&& futures) const;
	void waitIdle();

	Transfer m_transfer;
	Ref<Device> m_device;
};

// impl

template <typename Cont>
void VRAM::wait(Cont&& futures) const {
	for (Future& f : futures) {
		if (f.valid()) {
			f.get();
		}
	}
}
} // namespace le::graphics
