#pragma once
#include <core/hash.hpp>
#include <graphics/render/descriptor_set.hpp>
#include <graphics/render/pipeline.hpp>
#include <graphics/render/pipeline_spec.hpp>
#include <graphics/utils/defer.hpp>
#include <ktl/move_only_function.hpp>
#include <unordered_map>

namespace le::graphics {
class PipelineFactory {
  public:
	using Spec = PipelineSpec;
	using GetSpirV = ktl::move_only_function<SpirV(Hash)>;
	struct Hasher;

	static Spec spec(ShaderSpec shader, PFlags flags = pflags_all, VertexInputInfo vertexInput = {});
	static vk::UniqueShaderModule makeModule(vk::Device device, SpirV const& spirV);

	PipelineFactory(not_null<VRAM*> vram, GetSpirV&& getSpirV, Buffering buffering = Buffering::eDouble) noexcept;

	Pipeline get(Spec const& spec, vk::RenderPass renderPass);
	bool contains(Hash spec, vk::RenderPass renderPass) const;
	Spec const* find(Hash spec) const;
	std::size_t markStale(Hash shaderURI);

	std::size_t specCount() const noexcept { return m_storage.size(); }
	void clear() noexcept { m_storage.clear(); }
	std::size_t pipeCount(Hash spec) const noexcept;
	void clear(Hash spec) noexcept;

  private:
	struct Pipe {
		Defer<vk::Pipeline> pipeline;
		vk::PipelineLayout layout;
		mutable ShaderInput input;
		bool stale{};

		Pipeline pipe() const noexcept { return {&input, pipeline, layout}; }
	};
	struct Meta {
		SetPoolsData spd;
		Defer<vk::PipelineLayout> layout;
		std::vector<Defer<vk::DescriptorSetLayout>> setLayouts;
		std::vector<ktl::fixed_vector<vk::DescriptorSetLayoutBinding, 16>> bindings;
	};
	using PassMap = std::unordered_map<vk::RenderPass, Pipe>;
	struct SpecMap {
		PassMap map;
		Spec spec;
		Meta meta;
	};

	std::optional<Pipe> makePipe(SpecMap const& spec, vk::RenderPass renderPass) const;
	Meta makeMeta(ShaderSpec const& shader) const;

	using SpecHash = Hash;
	std::unordered_map<SpecHash, SpecMap> m_storage;
	GetSpirV m_getSpirV;
	not_null<VRAM*> m_vram;
	Buffering m_buffering;
};

struct PipelineFactory::Hasher {
	std::size_t operator()(Spec const& spec) const;
};
} // namespace le::graphics
