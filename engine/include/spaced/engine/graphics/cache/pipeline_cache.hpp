#pragma once
#include <spaced/engine/core/mono_instance.hpp>
#include <spaced/engine/core/not_null.hpp>
#include <spaced/engine/graphics/pipeline_state.hpp>
#include <spaced/engine/graphics/shader_layout.hpp>
#include <mutex>
#include <span>
#include <unordered_map>

namespace spaced::graphics {
class PipelineCache : public MonoInstance<PipelineCache> {
  public:
	explicit PipelineCache(ShaderLayout shader_layout = {});

	[[nodiscard]] auto shader_layout() const -> ShaderLayout const& { return m_shader_layout; }
	auto set_shader_layout(ShaderLayout shader_layout) -> void;

	[[nodiscard]] auto load(PipelineFormat format, Uri const& vert, Uri const& frag, PipelineState const& state = {}) -> vk::Pipeline;

	[[nodiscard]] auto pipeline_layout() const -> vk::PipelineLayout { return *m_pipeline_layout; }
	[[nodiscard]] auto descriptor_set_layouts() const -> std::span<vk::DescriptorSetLayout const> { return m_descriptor_set_layouts_view; }

  private:
	struct Key {
	  public:
		Key(PipelineFormat format, NotNull<Uri const*> vertex, NotNull<Uri const*> fragment, NotNull<PipelineState const*> state);

		[[nodiscard]] auto hash() const -> std::size_t { return cached_hash; }

		auto operator==(Key const& rhs) const -> bool { return hash() == rhs.hash(); }

		PipelineFormat format{};
		NotNull<Uri const*> vert;
		NotNull<Uri const*> frag;
		NotNull<PipelineState const*> state;
		std::size_t cached_hash{};
	};

	struct Hasher {
		auto operator()(Key const& key) const -> std::size_t { return key.hash(); }
	};

	mutable std::mutex m_mutex{};

	[[nodiscard]] auto build(Key const& key) -> vk::UniquePipeline;

	std::unordered_map<Key, vk::UniquePipeline, Hasher> m_map{};
	ShaderLayout m_shader_layout{};
	std::vector<vk::UniqueDescriptorSetLayout> m_descriptor_set_layouts{};
	std::vector<vk::DescriptorSetLayout> m_descriptor_set_layouts_view{};
	vk::UniquePipelineLayout m_pipeline_layout{};
	vk::Device m_device{};
};
} // namespace spaced::graphics
