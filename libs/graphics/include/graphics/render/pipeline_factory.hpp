#pragma once
#include <graphics/render/descriptor_set.hpp>
#include <graphics/render/pipeline_spec.hpp>
#include <graphics/utils/deferred.hpp>
#include <ktl/move_only_function.hpp>
#include <unordered_map>

namespace le::graphics {
struct PipelineData {
	not_null<ShaderInput*> shaderInput;
	vk::Pipeline pipeline;
	vk::PipelineLayout layout;
};

class PipelineFactory {
  public:
	using Data = PipelineData;
	using Spec = PipelineSpec;
	using GetShader = ktl::move_only_function<Shader const&(Hash)>;
	struct Hasher;

	static Spec spec(Hash shaderURI, PFlags flags = pflags_all, VertexInputInfo vertexInput = {});

	PipelineFactory(not_null<VRAM*> vram, GetShader&& getShader, Buffering buffering = 2_B) noexcept;

	Data get(Spec const& spec, vk::RenderPass renderPass);
	bool contains(Hash spec, vk::RenderPass renderPass) const;
	Spec const* find(Hash spec) const;
	std::size_t markStale(Hash shaderURI);

	std::size_t specCount() const noexcept { return m_storage.size(); }
	void clear() noexcept { m_storage.clear(); }
	std::size_t pipeCount(Hash spec) const noexcept;
	void clear(Hash spec) noexcept;

  private:
	struct Pipe {
		Deferred<vk::Pipeline> pipeline;
		vk::PipelineLayout layout;
		mutable ShaderInput input;
		bool stale{};

		Data data() const noexcept { return {&input, pipeline, layout}; }
	};
	struct Meta {
		SetPoolsData spd;
		Deferred<vk::PipelineLayout> layout;
		std::vector<Deferred<vk::DescriptorSetLayout>> setLayouts;
		std::vector<ktl::fixed_vector<BindingInfo, 16>> bindingInfos;
	};
	using PassMap = std::unordered_map<vk::RenderPass, Pipe>;
	struct SpecMap {
		PassMap map;
		Spec spec;
		Meta meta;
	};

	std::optional<Pipe> makePipe(SpecMap const& spec, vk::RenderPass renderPass) const;
	Meta makeMeta(Hash shaderURI) const;

	using SpecHash = Hash;
	std::unordered_map<SpecHash, SpecMap> m_storage;
	GetShader m_getShader;
	not_null<VRAM*> m_vram;
	Buffering m_buffering;
};

struct PipelineFactory::Hasher {
	std::size_t operator()(Spec const& spec) const;
};
} // namespace le::graphics
