#pragma once
#include <graphics/bitmap.hpp>
#include <graphics/buffer.hpp>
#include <graphics/context/transfer.hpp>
#include <graphics/image.hpp>
#include <graphics/image_ref.hpp>
#include <graphics/utils/command_pool.hpp>
#include <unordered_map>

namespace le::graphics {
class Device;
class CommandBuffer;
namespace utils {
class STBImg;
}

class VRAM final : public Memory {
  public:
	using notify_t = Transfer::notify_t;
	using Future = Transfer::Future;
	using Memory::blit;
	using Memory::copy;
	using Images = ktl::fixed_vector<utils::STBImg, 6>;

	struct Scratch {
		std::optional<Buffer> buffer;
		std::optional<Image> image;
	};

	template <typename T = bool>
	struct Op {
		Scratch scratch;
		T outcome;

		Op(T outcome = T{}) noexcept : outcome(outcome) {}
	};

	static constexpr AspectPair colour_aspects_v = {vIAFB::eColor, vIAFB::eColor};

	VRAM(not_null<Device*> device, Transfer::CreateInfo const& transferInfo = {});
	~VRAM();

	Buffer makeBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, bool hostVisible);
	Buffer makeStagingBuffer(vk::DeviceSize size) const { return m_transfer.makeStagingBuffer(size); }

	template <typename T>
	Buffer makeBO(T const& t, vk::BufferUsageFlags usage);

	[[nodiscard]] Future stage(Buffer& out_deviceBuffer, void const* pData, vk::DeviceSize size = 0);
	[[nodiscard]] Future copyAsync(Span<BmpView const> bitmaps, Image const& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects = vIAFB::eColor);
	[[nodiscard]] Future copyAsync(Images&& imgs, Image const& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects = vIAFB::eColor);

	bool blit(CommandBuffer cb, TPair<ImageRef> const& images, BlitFilter filter = BlitFilter::eLinear, AspectPair aspects = colour_aspects_v) const;
	bool copy(CommandBuffer cb, TPair<ImageRef> const& images, vk::ImageAspectFlags aspects = vIAFB::eColor) const;
	bool makeMipMaps(CommandBuffer cb, Image const& out_dst, LayoutPair fromTo, vk::ImageAspectFlags aspects = vIAFB::eColor) const;

	CommandPool& commandPool();

	template <typename Cont>
	void wait(Cont const& futures) const;
	void waitIdle();

	bool update(bool force = false);
	void shutdown();

	not_null<Device*> m_device;

  private:
	template <typename T>
	struct ImageCopier;

	Transfer m_transfer;
	ktl::strict_tmutex<std::unordered_map<std::thread::id, CommandPool>> m_commandPools;
	struct {
		vk::PipelineStageFlags stages = vk::PipelineStageFlagBits::eBottomOfPipe | vk::PipelineStageFlagBits::eVertexShader;
		vk::AccessFlags access = vk::AccessFlagBits::eShaderRead;
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
