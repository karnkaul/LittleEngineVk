#pragma once
#include <unordered_map>
#include <core/utils/vbase.hpp>
#include <dumb_json/json.hpp>
#include <engine/assets/asset_list.hpp>
#include <engine/assets/asset_loaders.hpp>
#include <engine/scene/draw_layer.hpp>

namespace le {
class AssetManifest : public utils::VBase {
  public:
	enum class Kind { eSampler, eShader, eTexture, ePipeline, eDrawLayer, eBitmapFont, eModel, eCOUNT_ };
	using Kinds = ktl::enum_flags<Kind, u16>;

	using StageID = AssetListLoader::StageID;
	using QueueID = AssetListLoader::QueueID;
	using Flag = AssetListLoader::Flag;
	using Flags = AssetListLoader::Flags;

	Flags& flags() noexcept { return m_loader.m_flags; }
	Flags const& flags() const noexcept { return m_loader.m_flags; }

	void append(AssetManifest const& rhs);
	std::size_t preload(dj::json_t const& root);
	void stage(dts::scheduler* scheduler);
	template <typename T>
	StageID stage(TAssetList<T> list, dts::scheduler* scheduler, Kinds kinds = {}, Span<StageID const> deps = {}, QueueID qid = {});
	std::size_t load(io::Path const& jsonID, dts::scheduler* scheduler);
	std::size_t unload(io::Path const& jsonID, dts::scheduler& scheduler);

	std::vector<StageID> deps(Kinds kinds) const noexcept;

	bool ready(dts::scheduler const& scheduler) const { return m_loader.ready(scheduler); }
	void wait(dts::scheduler& scheduler) const { m_loader.wait(scheduler); }

	EnumArray<Kind, QueueID> m_jsonQIDs = {};

  protected:
	graphics::Device& device();
	graphics::VRAM& vram();
	graphics::RenderContext& context();
	AssetStore& store();

  private:
	using Metadata = dj::ptr<dj::json_t>;
	using Group = std::unordered_map<io::Path, Metadata>;

	not_null<class Engine*> engine();

	std::size_t add(std::string_view name, Group group);
	std::size_t addSamplers(Group group);
	std::size_t addShaders(Group group);
	std::size_t addTextures(Group group);
	std::size_t addPipelines(Group group);
	std::size_t addDrawLayers(Group group);
	std::size_t addFonts(Group group);
	std::size_t addBitmapFonts(Group group);
	std::size_t addModels(Group group);
	std::size_t unload();
	template <typename T, typename U>
	std::size_t unload(U& cont);

	virtual std::size_t addCustom(std::string_view, Group) { return 0; }
	virtual void loadCustom(dts::scheduler*) {}
	virtual std::size_t unloadCustom() { return 0; }

	AssetList<graphics::Sampler> m_samplers;
	AssetLoadList<graphics::Shader> m_shaders;
	AssetLoadList<graphics::Pipeline> m_pipelines;
	AssetList<DrawLayer> m_drawLayers;
	AssetLoadList<graphics::Texture> m_textures;
	AssetLoadList<BitmapFont> m_bitmapFonts;
	AssetLoadList<Model> m_models;
	AssetListLoader m_loader;
	EnumArray<Kind, StageID> m_deps = {};
	class Engine* m_engine{};
};

// impl

template <typename T>
AssetManifest::StageID AssetManifest::stage(TAssetList<T> lists, dts::scheduler* scheduler, Kinds kinds, Span<StageID const> deps, QueueID qid) {
	auto dp = this->deps(kinds);
	dp.reserve(dp.size() + deps.size());
	std::copy(deps.begin(), deps.end(), std::back_inserter(dp));
	return m_loader.stage(std::move(lists), scheduler, dp, qid);
}
} // namespace le
