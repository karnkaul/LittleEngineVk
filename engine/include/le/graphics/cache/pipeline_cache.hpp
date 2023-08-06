#pragma once
#include <le/core/mono_instance.hpp>
#include <le/core/not_null.hpp>
#include <le/graphics/cache/shader_cache.hpp>
#include <le/graphics/pipeline_state.hpp>
#include <le/graphics/shader.hpp>
#include <le/graphics/shader_layout.hpp>
#include <span>
#include <unordered_map>

namespace le::graphics {
class PipelineCache : public MonoInstance<PipelineCache> {
  public:
	explicit PipelineCache(ShaderLayout shader_layout = {});

	[[nodiscard]] auto shader_layout() const -> ShaderLayout const& { return m_shader_layout; }
	auto set_shader_layout(ShaderLayout shader_layout) -> void;

	[[nodiscard]] auto load(PipelineFormat format, Shader shader, PipelineState state = {}) -> vk::Pipeline;

	[[nodiscard]] auto pipeline_layout() const -> vk::PipelineLayout { return *m_pipeline_layout; }
	[[nodiscard]] auto descriptor_set_layouts() const -> std::span<vk::DescriptorSetLayout const> { return m_descriptor_set_layouts_view; }

	[[nodiscard]] auto shader_count() const -> std::size_t { return m_shader_cache.shader_count(); }
	[[nodiscard]] auto pipeline_count() const -> std::size_t { return m_pipelines.size(); }

	auto clear_pipelines() -> void;
	auto clear_pipelines_and_shaders() -> void;

  private:
	struct Key {
	  public:
		Key(PipelineFormat format, Shader shader, PipelineState state);

		[[nodiscard]] auto hash() const -> std::size_t { return cached_hash; }

		auto operator==(Key const& rhs) const -> bool { return hash() == rhs.hash(); }

		PipelineFormat format{};
		Shader shader{};
		PipelineState state{};
		std::size_t cached_hash{};
	};

	struct Hasher {
		auto operator()(Key const& key) const -> std::size_t { return key.hash(); }
	};

	[[nodiscard]] auto build(Key const& key) -> vk::UniquePipeline;

	std::unordered_map<Key, vk::UniquePipeline, Hasher> m_pipelines{};
	ShaderCache m_shader_cache{};
	ShaderLayout m_shader_layout{};
	std::vector<vk::UniqueDescriptorSetLayout> m_descriptor_set_layouts{};
	std::vector<vk::DescriptorSetLayout> m_descriptor_set_layouts_view{};
	vk::UniquePipelineLayout m_pipeline_layout{};
	vk::Device m_device{};
};
} // namespace le::graphics
