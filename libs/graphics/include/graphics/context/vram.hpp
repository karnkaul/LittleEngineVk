#pragma once
#include <core/utils/future.hpp>
#include <graphics/bitmap.hpp>
#include <graphics/context/transfer.hpp>

namespace le::graphics {
class Device;

class VRAM final : public Memory {
  public:
	using notify_t = Transfer::notify_t;
	using Future = ::le::utils::Future<notify_t>;
	using Memory::blit;
	using Memory::copy;

	VRAM(not_null<Device*> device, Transfer::CreateInfo const& transferInfo = {});
	~VRAM();

	Buffer makeBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, bool bHostVisible);
	template <typename T>
	Buffer makeBO(T const& t, vk::BufferUsageFlags usage);

	[[nodiscard]] Future copy(Buffer const& src, Buffer& out_dst, vk::DeviceSize size = 0);
	[[nodiscard]] Future stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size = 0);
	[[nodiscard]] Future copy(Span<BMPview const> bitmaps, Image& out_dst, LayoutPair layouts);
	[[nodiscard]] Future blit(Image const& src, Image& out_dst, LayoutPair layouts, TPair<vk::ImageAspectFlags> aspects,
							  vk::Filter filter = vk::Filter::eLinear);

	template <typename Cont>
	void wait(Cont const& futures) const;
	void waitIdle();

	bool update(bool force = false);

	not_null<Device*> m_device;

  private:
	void shutdown();

	Transfer m_transfer;
	struct {
		vk::PipelineStageFlags stages;
		vk::AccessFlags access;
	} m_post;

	friend class Swapchain;
};

// impl

template <typename T>
Buffer VRAM::makeBO(T const& t, vk::BufferUsageFlags usage) {
	Buffer ret = makeBuffer(sizeof(T), usage, true);
	ret.writeT(t);
	return ret;
}

template <typename Cont>
void VRAM::wait(Cont const& futures) const {
	for (Future const& f : futures) { f.wait(); }
}
} // namespace le::graphics
