#pragma once
#include <unordered_map>
#include <core/utils/vbase.hpp>
#include <dumb_json/json.hpp>
#include <engine/assets/asset_list.hpp>
#include <engine/assets/asset_loaders.hpp>
#include <kt/enum_flags/enum_flags.hpp>

namespace le {
class AssetManifest : public utils::VBase {
  public:
	enum class Kind { eSampler, eShader, eTexture, ePipeline, eBitmapFont, eModel, eCOUNT_ };
	using Flags = kt::enum_flags<Kind>;

	using StageID = AssetListLoader::StageID;

	void loaderFlags(AssetListLoader::Flags flags) noexcept { m_loader.m_flags = flags; }
	void append(AssetManifest const& rhs);
	std::size_t preload(dj::json_t const& root);
	void stage(dts::scheduler* scheduler);
	template <typename T>
	StageID stage(TAssetList<T> list, dts::scheduler* scheduler, Flags flags = {}, Span<StageID const> deps = {});
	std::size_t load(io::Path const& jsonID, dts::scheduler* scheduler);

	std::vector<StageID> deps(Flags flags) const noexcept;

	bool ready(dts::scheduler const& scheduler) const { return m_loader.ready(scheduler); }
	void wait(dts::scheduler& scheduler) const { m_loader.wait(scheduler); }

  protected:
	not_null<graphics::Device*> device();
	not_null<graphics::VRAM*> vram();

  private:
	struct Metadata {
		io::Path id;
		dj::ptr<dj::json_t> json;
	};
	using Group = std::vector<Metadata>;

	std::size_t add(std::string_view name, Group group);
	std::size_t addSamplers(Group group);
	std::size_t addShaders(Group group);
	std::size_t addTextures(Group group);
	std::size_t addFonts(Group group);
	std::size_t addBitmapFonts(Group group);
	std::size_t addModels(Group group);

	virtual std::size_t addCustom(Group) { return 0; }
	virtual void loadCustom(dts::scheduler*) {}

	AssetList<graphics::Sampler> m_samplers;
	AssetLoadList<graphics::Shader> m_shaders;
	AssetLoadList<graphics::Pipeline> m_pipelines;
	AssetLoadList<graphics::Texture> m_textures;
	AssetLoadList<BitmapFont> m_bitmapFonts;
	AssetLoadList<Model> m_models;
	AssetListLoader m_loader;
	EnumArray<Kind, StageID> m_deps = {};
	graphics::Device* m_device{};
	graphics::VRAM* m_vram{};
};

// impl

template <typename T>
AssetManifest::StageID AssetManifest::stage(TAssetList<T> lists, dts::scheduler* scheduler, Flags flags, Span<StageID const> deps) {
	auto dp = this->deps(flags);
	std::copy(deps.begin(), deps.end(), std::back_inserter(dp));
	return m_loader.stage(std::move(lists), scheduler, dp);
}
} // namespace le
