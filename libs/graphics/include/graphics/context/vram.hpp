#pragma once
#include <graphics/bitmap.hpp>
#include <graphics/context/transfer.hpp>
#include <graphics/render/target.hpp>

namespace le::graphics {
class Device;
class CommandBuffer;

class VRAM final : public Memory {
  public:
	using notify_t = Transfer::notify_t;
	using Future = Transfer::Future;
	using Memory::blit;
	using Memory::copy;

	static constexpr LayoutPair blit_layouts_v = {vIL::eTransferSrcOptimal, vIL::eTransferDstOptimal};
	static constexpr AspectPair colour_aspects_v = {vIAFB::eColor, vIAFB::eColor};

	VRAM(not_null<Device*> device, Transfer::CreateInfo const& transferInfo = {});
	~VRAM();

	Buffer makeBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, bool bHostVisible);
	template <typename T>
	Buffer makeBO(T const& t, vk::BufferUsageFlags usage);

	[[nodiscard]] Future copy(Buffer const& src, Buffer& out_dst, vk::DeviceSize size = 0);
	[[nodiscard]] Future stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size = 0);
	[[nodiscard]] Future copy(Span<ImgView const> bitmaps, Image& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects = vIAFB::eColor);
	bool blit(CommandBuffer cb, Image const& src, Image& out_dst, vk::Filter filter = vk::Filter::eLinear, AspectPair aspects = colour_aspects_v) const;
	bool blit(CommandBuffer cb, TPair<RenderTarget> images, vk::Filter filter = vk::Filter::eLinear, AspectPair aspects = colour_aspects_v) const;

	template <typename Cont>
	void wait(Cont const& futures) const;
	void waitIdle();

	bool update(bool force = false);
	void shutdown();

	not_null<Device*> m_device;

  private:
	Transfer m_transfer;
	struct {
		vk::PipelineStageFlags stages = vk::PipelineStageFlagBits::eBottomOfPipe;
		vk::AccessFlags access;
	} m_post;
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
