#pragma once
#include <levk/graphics/command_buffer.hpp>
#include <levk/graphics/render/descriptor_set.hpp>
#include <levk/graphics/render/draw_list.hpp>
#include <optional>

namespace le::graphics {
struct DescriptorFallback {
	not_null<Texture const*> texture;
	not_null<Texture const*> cubemap;
};

class DescriptorHelper {
  public:
	class Updater;

	DescriptorHelper(DescriptorFallback fallback, vk::PipelineLayout layout, ShaderInput& input, CommandBuffer cb) noexcept
		: m_fallback(fallback), m_cb(cb), m_input(input), m_layout(layout) {}

	bool contains(u32 setNumber);
	std::optional<Updater> nextSet(u32 setNumber);

	CommandBuffer const& commandBuffer() const noexcept { return m_cb; }
	vk::PipelineLayout pipelineLayout() const noexcept { return m_layout; }
	ShaderInput const& shaderInput() const noexcept { return m_input; }

  private:
	void bind(DescriptorSet const& set) const;

	std::size_t m_nextIndex[max_bindings_v] = {};
	DescriptorFallback m_fallback;
	CommandBuffer m_cb;
	ShaderInput& m_input;
	vk::PipelineLayout m_layout;
};

using DescriptorUpdater = DescriptorHelper::Updater;

class DescriptorHelper::Updater {
  public:
	Updater(DescriptorFallback fallback, DescriptorSet& descriptorSet, DescriptorHelper& helper);
	~Updater();

	DescriptorSet const& descriptorSet() const noexcept { return m_descriptorSet; }

	template <typename T>
	bool update(u32 binding, T const& t, vk::DescriptorType type = vk::DescriptorType::eUniformBuffer);
	bool update(u32 binding, Ptr<Texture const> tex);
	bool update(u32 binding, ShaderBuffer const& buffer);

  private:
	bool check(u32 binding, vk::DescriptorType const* type = {}, Texture::Type const* texType = {});
	Texture const& safeTex(Texture const* tex, u32 bind) const;

	DescriptorFallback m_fallback;
	ktl::fixed_vector<u32, max_bindings_v> m_binds;
	DescriptorSet& m_descriptorSet;
	DescriptorHelper& m_helper;
};

// impl

template <typename T>
bool DescriptorUpdater::update(u32 bind, T const& t, vk::DescriptorType type) {
	if (check(bind, &type)) {
		m_descriptorSet.writeUpdate(t, bind);
		return true;
	}
	return false;
}
} // namespace le::graphics
