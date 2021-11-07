#pragma once
#include <core/hash.hpp>
#include <core/not_null.hpp>
#include <core/utils/error.hpp>
#include <glm/vec2.hpp>
#include <graphics/common.hpp>
#include <graphics/qflags.hpp>
#include <ktl/fixed_vector.hpp>
#include <atomic>
#include <vector>

namespace le::graphics {
class Device;
class Pipeline;
class DescriptorSet;
class Buffer;
class Image;

class CommandBuffer {
  public:
	using vBP = vk::PipelineBindPoint;
	using Layouts = TPair<vk::ImageLayout>;
	using Access = TPair<vk::AccessFlags>;
	using Stages = TPair<vk::PipelineStageFlags>;

	struct PassInfo {
		ktl::fixed_vector<vk::ClearValue, 2> clearValues;
		vk::CommandBufferUsageFlags usage;
		vk::SubpassContents subpassContents = vk::SubpassContents::eInline;
	};

	inline static auto s_drawCalls = std::atomic<u32>(0);

	static std::vector<CommandBuffer> make(not_null<Device*> device, vk::CommandPool pool, u32 count);
	static void make(std::vector<CommandBuffer>& out, not_null<Device*> device, vk::CommandPool pool, u32 count);

	CommandBuffer() = default;
	CommandBuffer(vk::CommandBuffer cmd);
	CommandBuffer(Device& device, vk::CommandPool cmd);

	void begin(vk::CommandBufferUsageFlags usage);
	void beginRenderPass(vk::RenderPass renderPass, vk::Framebuffer framebuffer, Extent2D extent, PassInfo const& info);
	void setViewport(vk::Viewport viewport) const;
	void setScissor(vk::Rect2D scissor) const;
	void setViewportScissor(vk::Viewport viewport, vk::Rect2D scissor) const;

	void bindPipe(Pipeline const& pipeline, Hash variant = Hash()) const;
	void bind(vk::Pipeline pipeline, vBP bindPoint = vBP::eGraphics) const;
	void bindSets(vk::PipelineLayout layout, vAP<vk::DescriptorSet> sets, u32 firstSet = 0, vAP<u32> offsets = {}, vBP bindPoint = vBP::eGraphics) const;
	void bindSet(vk::PipelineLayout layout, DescriptorSet const& set) const;
	template <typename T>
	void push(vk::PipelineLayout layout, vk::ShaderStageFlags stages, u32 offset, vAP<T> pushConstants) const;
	void bindVBOs(u32 first, vAP<vk::Buffer> buffers, vAP<vk::DeviceSize> offsets) const;
	void bindIBO(vk::Buffer buffer, vk::DeviceSize offset = vk::DeviceSize(0), vk::IndexType indexType = vk::IndexType::eUint32) const;
	void bindVBO(Buffer const& vbo, Buffer const* pIbo = nullptr) const;
	void drawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstInstance = 0, s32 vertexOffset = 0, u32 firstIndex = 0) const;
	void draw(u32 vertexCount, u32 instanceCount = 1, u32 firstInstance = 0, u32 firstVertex = 0) const;

	void transitionImage(Image const& image, vk::ImageAspectFlags aspect, Layouts transition, Access access, Stages stages) const;
	void transitionImage(vk::Image image, u32 layerCount, vk::ImageAspectFlags aspect, Layouts transition, Access access, Stages stages) const;

	void endRenderPass();
	void end();

	bool valid() const noexcept { return m_cb != vk::CommandBuffer(); }
	bool recording() const noexcept { return valid() && m_flags.test(Flag::eRecording); }
	bool rendering() const noexcept { return valid() && m_flags.all(Flags(Flag::eRecording) | Flag::eRendering); }

	vk::CommandBuffer m_cb;

  private:
	enum class Flag { eRecording, eRendering };
	using Flags = ktl::enum_flags<Flag, u8>;
	Flags m_flags;
};

// impl

template <typename T>
void CommandBuffer::push(vk::PipelineLayout layout, vk::ShaderStageFlags stages, u32 offset, vAP<T> pushConstants) const {
	ENSURE(rendering(), "Command buffer not recording!");
	m_cb.pushConstants<T>(layout, stages, offset, pushConstants);
}
} // namespace le::graphics
