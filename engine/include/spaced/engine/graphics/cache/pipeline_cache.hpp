#pragma once
#include <spaced/engine/core/mono_instance.hpp>
#include <spaced/engine/core/not_null.hpp>
#include <spaced/engine/graphics/pipeline_state.hpp>
#include <spaced/engine/graphics/shader_layout.hpp>
#include <mutex>
#include <span>
#include <unordered_map>

namespace spaced::graphics {
struct PipelineKey {
	PipelineFormat format{};
	NotNull<Uri const*> vertex_shader;
	NotNull<Uri const*> fragment_shader;
	NotNull<PipelineState const*> state;

	mutable std::size_t cached_hash_{};

	[[nodiscard]] auto hash() const -> std::size_t;

	auto operator==(PipelineKey const& rhs) const -> bool { return hash() == rhs.hash(); }
};

class PipelineCache : public MonoInstance<PipelineCache> {
  public:
	explicit PipelineCache(ShaderLayout shader_layout = {});

	[[nodiscard]] auto shader_layout() const -> ShaderLayout const& { return m_shader_layout; }
	auto set_shader_layout(ShaderLayout shader_layout) -> void;

	[[nodiscard]] auto load(PipelineFormat format, Uri const& vert, Uri const& frag, PipelineState const& state = {}) -> vk::Pipeline;

	[[nodiscard]] auto pipeline_layout() const -> vk::PipelineLayout { return *m_pipeline_layout; }
	[[nodiscard]] auto descriptor_set_layouts() const -> std::span<vk::DescriptorSetLayout const> { return m_descriptor_set_layouts_view; }

  private:
	struct Hasher {
		auto operator()(PipelineKey const& key) const -> std::size_t { return key.hash(); }
	};

	mutable std::mutex m_mutex{};

	[[nodiscard]] auto build(PipelineKey const& key) -> vk::UniquePipeline;

	std::unordered_map<PipelineKey, vk::UniquePipeline, Hasher> m_map{};
	ShaderLayout m_shader_layout{};
	std::vector<vk::UniqueDescriptorSetLayout> m_descriptor_set_layouts{};
	std::vector<vk::DescriptorSetLayout> m_descriptor_set_layouts_view{};
	vk::UniquePipelineLayout m_pipeline_layout{};
	vk::Device m_device{};
};
} // namespace spaced::graphics
