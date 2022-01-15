#pragma once
#include <dumb_json/json.hpp>
#include <levk/core/utils/vbase.hpp>
#include <levk/engine/assets/asset_list.hpp>
#include <levk/engine/assets/asset_loaders.hpp>
#include <levk/engine/render/pipeline.hpp>
#include <unordered_map>

namespace le {
class Skybox;

class AssetManifest : public utils::VBase {
  public:
	enum class Kind {
		eSampler,
		eSpirV,
		eTexture,
		eRenderLayer,
		eRenderPipeline,
		eFont,
		eMaterial,
		eSkybox,
		eModel,
		eCOUNT_,
	};
	using Kinds = ktl::enum_flags<Kind, u16>;

	using StageID = AssetListLoader::StageID;
	using QueueID = AssetListLoader::QueueID;
	using Flag = AssetListLoader::Flag;
	using Flags = AssetListLoader::Flags;

	Flags& flags() noexcept { return m_loader.m_flags; }
	Flags const& flags() const noexcept { return m_loader.m_flags; }

	std::size_t preload(dj::json const& root);
	void stage(dts::scheduler* scheduler);
	template <typename T>
	StageID stage(TAssetList<T> list, dts::scheduler* scheduler, Kinds kinds = {}, Span<StageID const> deps = {}, QueueID qid = {});
	std::size_t load(io::Path const& jsonID, dts::scheduler* scheduler);
	std::size_t unload(io::Path const& jsonID, dts::scheduler& scheduler);

	std::vector<StageID> deps(Kinds kinds) const noexcept;

	bool ready(dts::scheduler const& scheduler) const { return m_loader.ready(scheduler); }
	void wait(dts::scheduler& scheduler) const { m_loader.wait(scheduler); }
	f32 progress() const noexcept { return m_loader.progress(); }

	EnumArray<Kind, QueueID> m_jsonQIDs = {};

  protected:
	graphics::Device& device();
	graphics::VRAM& vram();
	graphics::RenderContext& context();
	AssetStore& store();

	template <typename T>
	std::size_t unloadMap(T& map);

  private:
	using Metadata = dj::ptr<dj::json>;
	using Group = std::unordered_map<std::string, Metadata>;

	virtual std::size_t addCustom(std::string_view, Group) { return 0; }
	virtual void loadCustom(dts::scheduler*) {}
	virtual std::size_t unloadCustom() { return 0; }

	not_null<class Engine*> engine();

	std::size_t preload(io::Path const& jsonID);
	std::size_t add(std::string_view name, Group group);
	std::size_t addSamplers(Group group);
	std::size_t addSpirV(Group group);
	std::size_t addTextures(Group group);
	std::size_t addRenderLayers(Group group);
	std::size_t addRenderPipelines(Group group);
	std::size_t addFonts(Group group);
	std::size_t addMaterials(Group group);
	std::size_t addSkyboxes(Group group);
	std::size_t addModels(Group group);
	std::size_t unload();

	AssetList<graphics::Sampler> m_samplers;
	AssetLoadList<graphics::SpirV> m_spirV;
	AssetLoadList<graphics::Texture> m_textures;
	AssetList<RenderLayer> m_renderLayers;
	AssetList<RenderPipeline> m_renderPipelines;
	AssetLoadList<graphics::Font> m_fonts;
	AssetList<Material> m_materials;
	AssetList<Skybox> m_skyboxes;
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

template <typename T>
std::size_t AssetManifest::unloadMap(T& out_map) {
	std::size_t ret{};
	for (auto const& [uri, _] : out_map) {
		if (store().unload(uri)) { ++ret; }
	}
	out_map.clear();
	return ret;
}
} // namespace le
