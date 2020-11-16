#pragma once
#include <vector>
#include <core/ensure.hpp>
#include <core/view.hpp>
#include <graphics/types.hpp>
#include <kt/enum_flags/enum_flags.hpp>

namespace le::graphics {
class Device;
class Pipeline;
class DescriptorSet;

class CommandBuffer {
  public:
	using vBP = vk::PipelineBindPoint;

	struct PassInfo {
		std::vector<vk::ClearValue> clearValues;
		vk::SubpassContents subpassContents = vk::SubpassContents::eInline;
		vk::CommandBufferUsageFlags usage = {};
	};

	static std::vector<CommandBuffer> make(Device& device, vk::CommandPool pool, u32 count, bool bSecondary);

	CommandBuffer() = default;
	CommandBuffer(vk::CommandBuffer cmd, vk::CommandPool pool);

	bool begin(vk::CommandBufferUsageFlags usage);
	bool begin(vk::RenderPass renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent, PassInfo const& info);
	void setViewportScissor(vk::Viewport viewport, vk::Rect2D scissor) const;

	void bindPipe(Pipeline const& pipeline) const;
	void bind(vk::Pipeline pipeline, vBP bindPoint = vBP::eGraphics) const;
	void bindSets(vk::PipelineLayout layout, vAP<vk::DescriptorSet> sets, u32 firstSet = 0, vAP<u32> offsets = {}, vBP bindPoint = vBP::eGraphics) const;
	template <typename T>
	void push(vk::PipelineLayout layout, vk::ShaderStageFlags stages, u32 offset, vAP<T> pushConstants) const;
	void bindVBOs(u32 first, vAP<vk::Buffer> buffers, vAP<vk::DeviceSize> offsets) const;
	void bindIBO(vk::Buffer buffer, vk::DeviceSize offset = vk::DeviceSize(0), vk::IndexType indexType = vk::IndexType::eUint32) const;
	void bindVBO(CView<Buffer> vbo, CView<Buffer> ibo) const;
	void drawIndexed(u32 indexCount, u32 instanceCount = 1, u32 firstIndex = 0, s32 vertexOffset = 0, u32 firstInstance = 0) const;
	void draw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0) const;

	void end();

	bool valid() const noexcept;
	bool recording() const noexcept;
	bool rendering() const noexcept;

	vk::CommandBuffer m_cmd;
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
	m_cmd.pushConstants<T>(layout, stages, offset, pushConstants);
}
inline bool CommandBuffer::valid() const noexcept {
	return !default_v(m_cmd);
}
inline bool CommandBuffer::recording() const noexcept {
	return valid() && m_flags.test(Flag::eRecording);
}

inline bool CommandBuffer::rendering() const noexcept {
	return valid() && m_flags.all(Flag::eRecording | Flag::eRendering);
}
} // namespace le::graphics
