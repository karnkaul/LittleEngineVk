#pragma once
#include <atomic>
#include <vector>
#include <core/ensure.hpp>
#include <core/hash.hpp>
#include <core/ref.hpp>
#include <kt/enum_flags/enum_flags.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
class Device;
class Pipeline;
class DescriptorSet;
class Buffer;
class Image;

class CommandBuffer {
  public:
	template <typename T>
	using vAP = vk::ArrayProxy<T const> const&;
	using vBP = vk::PipelineBindPoint;
	template <typename T>
	using Duet = std::pair<T, T>;
	using Layouts = Duet<vk::ImageLayout>;
	using Access = Duet<vk::AccessFlags>;
	using Stages = Duet<vk::PipelineStageFlags>;

	struct PassInfo {
		kt::fixed_vector<vk::ClearValue, 2> clearValues;
		vk::SubpassContents subpassContents = vk::SubpassContents::eInline;
		vk::CommandBufferUsageFlags usage = {};
	};

	inline static auto s_drawCalls = std::atomic<u32>(0);

	static std::vector<CommandBuffer> make(Device& device, vk::CommandPool pool, u32 count, bool bSecondary);

	CommandBuffer() = default;
	CommandBuffer(vk::CommandBuffer cmd, vk::CommandPool pool);

	bool begin(vk::CommandBufferUsageFlags usage);
	bool begin(vk::RenderPass renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent, PassInfo const& info);
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

	bool valid() const noexcept;
	bool recording() const noexcept;
	bool rendering() const noexcept;

	vk::CommandBuffer m_cb;
	vk::CommandPool m_pool;

  private:
	enum class Flag { eRecording, eRendering, eCOUNT_ };
	using Flags = kt::enum_flags<Flag>;
	Flags m_flags;
};

// impl

template <typename T>
void CommandBuffer::push(vk::PipelineLayout layout, vk::ShaderStageFlags stages, u32 offset, vAP<T> pushConstants) const {
	ENSURE(rendering(), "Command buffer not recording!");
	m_cb.pushConstants<T>(layout, stages, offset, pushConstants);
}
inline bool CommandBuffer::valid() const noexcept {
	return m_cb != vk::CommandBuffer();
}
inline bool CommandBuffer::recording() const noexcept {
	return valid() && m_flags.test(Flag::eRecording);
}

inline bool CommandBuffer::rendering() const noexcept {
	return valid() && m_flags.all(Flags(Flag::eRecording) | Flag::eRendering);
}
} // namespace le::graphics
