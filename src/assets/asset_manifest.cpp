#include <engine/assets/asset_manifest.hpp>
#include <engine/engine.hpp>
#include <engine/render/skybox.hpp>
#include <engine/utils/logger.hpp>
#include <graphics/context/bootstrap.hpp>
#include <ktl/enum_flags/enumerate_enum.hpp>
#include <ktl/stack_string.hpp>

namespace le {
namespace {
graphics::PFlags parseFlags(Span<std::string const> arr) {
	graphics::PFlags ret;
	for (std::string const& str : arr) {
		if (str == "all") { return graphics::pflags_all; }
		if (str == "depth_test") { ret.set(graphics::PFlag::eDepthTest); }
		if (str == "depth_write") { ret.set(graphics::PFlag::eDepthWrite); }
		if (str == "alpha_blend") { ret.set(graphics::PFlag::eAlphaBlend); }
	}
	return ret;
}

vk::PrimitiveTopology parseTopology(std::string_view text) noexcept {
	if (text == "point_list") {
		return vk::PrimitiveTopology::ePointList;
	} else if (text == "line_list") {
		return vk::PrimitiveTopology::eLineList;
	} else if (text == "line_strip") {
		return vk::PrimitiveTopology::eLineStrip;
	} else if (text == "triangle_strip") {
		return vk::PrimitiveTopology::eTriangleStrip;
	}
	return vk::PrimitiveTopology::eTriangleList;
}

vk::PolygonMode parsePolygonMode(std::string_view text) noexcept {
	if (text == "line") {
		return vk::PolygonMode::eLine;
	} else if (text == "point") {
		return vk::PolygonMode::ePoint;
	}
	return vk::PolygonMode::eFill;
}
} // namespace

void AssetManifest::append(AssetManifest const& rhs) {
	m_samplers = m_samplers + rhs.m_samplers;
	m_textures = m_textures + rhs.m_textures;
}

std::size_t AssetManifest::preload(dj::json const& root) {
	std::size_t ret{};
	for (auto const& [groupName, entries] : root.as<dj::map_t>()) {
		Group group;
		for (auto const& json : entries->as<dj::vec_t>()) {
			if (auto uri = json->find_as<std::string>("uri")) { group.insert({std::move(*uri), json}); }
		}
		if (group.empty()) {
			for (auto& uri : entries->as<std::vector<std::string>>()) { group.insert({std::move(uri), std::make_shared<dj::json>()}); }
		}
		if (!group.empty()) { ret += add(groupName, std::move(group)); }
	}
	return ret;
}

void AssetManifest::stage(dts::scheduler* scheduler) {
	StageID const matDeps[] = {m_deps[Kind::eTexture], m_deps[Kind::ePipelineState]};
	static constexpr auto start_ = __LINE__;
	m_deps[Kind::eSampler] = m_loader.stage(std::move(m_samplers), scheduler, {}, m_jsonQIDs[Kind::eSampler]);
	m_deps[Kind::eTexture] = m_loader.stage(std::move(m_textures), scheduler, m_deps[Kind::eSampler], m_jsonQIDs[Kind::eTexture]);
	m_deps[Kind::eSpirV] = m_loader.stage(std::move(m_spirV), scheduler, {}, m_jsonQIDs[Kind::eSpirV]);
	m_deps[Kind::ePipelineState] = m_loader.stage(std::move(m_pipelineStates), scheduler, m_deps[Kind::eSpirV], m_jsonQIDs[Kind::ePipelineState]);
	m_deps[Kind::eRenderLayer] = m_loader.stage(std::move(m_renderLayers), scheduler, m_deps[Kind::ePipelineState], m_jsonQIDs[Kind::eRenderLayer]);
	m_deps[Kind::eMaterial] = m_loader.stage(std::move(m_materials), scheduler, matDeps, m_jsonQIDs[Kind::eMaterial]);
	m_deps[Kind::eBitmapFont] = m_loader.stage(std::move(m_bitmapFonts), scheduler, m_deps[Kind::eTexture], m_jsonQIDs[Kind::eBitmapFont]);
	m_deps[Kind::eSkybox] = m_loader.stage(std::move(m_skyboxes), scheduler, m_deps[Kind::eTexture], m_jsonQIDs[Kind::eSkybox]);
	m_deps[Kind::eModel] = m_loader.stage(std::move(m_models), scheduler, {}, m_jsonQIDs[Kind::eModel]);
	static_assert(__LINE__ - start_ - 1 == int(Kind::eCOUNT_));
	loadCustom(scheduler);
}

std::size_t AssetManifest::load(io::Path const& jsonID, dts::scheduler* scheduler) {
	if (auto const ret = preload(jsonID)) {
		stage(scheduler);
		return ret;
	}
	return 0U;
}

std::size_t AssetManifest::unload(io::Path const& jsonID, dts::scheduler& scheduler) {
	wait(scheduler);
	if (preload(jsonID) > 0) { return unload(); }
	return 0U;
}

std::vector<AssetManifest::StageID> AssetManifest::deps(Kinds kinds) const noexcept {
	std::vector<StageID> ret;
	ret.reserve(std::size_t(Kind::eCOUNT_));
	for (Kind const k : ktl::enumerate_enum<Kind>()) {
		if (kinds.test(k)) { ret.push_back(m_deps[k]); }
	}
	return ret;
}

graphics::Device& AssetManifest::device() { return engine()->gfx().boot.device; }
graphics::VRAM& AssetManifest::vram() { return engine()->gfx().boot.vram; }
graphics::RenderContext& AssetManifest::context() { return engine()->gfx().context; }
AssetStore& AssetManifest::store() { return engine()->store(); }

not_null<Engine*> AssetManifest::engine() { return m_engine ? m_engine : (m_engine = Services::get<Engine>()); }

std::size_t AssetManifest::preload(io::Path const& jsonID) {
	std::size_t ret{};
	if (auto eng = Services::find<Engine>()) {
		dj::json json;
		auto& resources = eng->store().resources();
		io::Path uris[] = {jsonID, jsonID + ".manifest"};
		if (auto res = resources.loadFirst(uris, Resource::Type::eText); res && json.read(res->string())) { ret = preload(json); }
	}
	return ret;
}

std::size_t AssetManifest::add(std::string_view groupName, Group group) {
	static constexpr auto start_ = __LINE__;
	if (groupName == "samplers") { return addSamplers(std::move(group)); }
	if (groupName == "shaders") { return addSpirV(std::move(group)); }
	if (groupName == "textures") { return addTextures(std::move(group)); }
	if (groupName == "models") { return addModels(std::move(group)); }
	if (groupName == "materials") { return addMaterials(std::move(group)); }
	if (groupName == "skyboxes") { return addSkyboxes(std::move(group)); }
	if (groupName == "bitmap_fonts") { return addBitmapFonts(std::move(group)); }
	if (groupName == "pipelines") { return addPipelineStates(std::move(group)); }
	if (groupName == "layers") { return addRenderLayers(std::move(group)); }
	static_assert(__LINE__ - start_ - 1 == int(Kind::eCOUNT_));
	return addCustom(groupName, std::move(group));
}

std::size_t AssetManifest::addSamplers(Group group) {
	std::size_t ret{};
	auto& dv = device();
	for (auto& [uri, json] : group) {
		if (json->contains("min") && json->contains("mag")) {
			graphics::TPair<vk::Filter> minMag;
			minMag.first = (*json)["min"].as<std::string_view>() == "nearest" ? vk::Filter::eNearest : vk::Filter::eLinear;
			minMag.second = (*json)["mag"].as<std::string_view>() == "nearest" ? vk::Filter::eNearest : vk::Filter::eLinear;
			vk::SamplerCreateInfo createInfo;
			if (auto mipMode = json->find_as<std::string_view>("mip_mode")) {
				auto const mm = *mipMode == "nearest" ? vk::SamplerMipmapMode::eNearest : vk::SamplerMipmapMode::eLinear;
				createInfo = graphics::Sampler::info(minMag, mm);
			} else {
				createInfo = graphics::Sampler::info(minMag);
			}
			m_samplers.add(std::move(uri), [createInfo, &dv]() { return std::make_unique<graphics::Sampler>(&dv, createInfo); });
			++ret;
		}
	}
	return ret;
}

namespace {
graphics::ShaderType parseShaderType(std::string_view str) noexcept {
	if (str == "vert" || str == "vertex") {
		return graphics::ShaderType::eVertex;
	} else if (str == "comp" || str == "compute") {
		return graphics::ShaderType::eCompute;
	}
	return graphics::ShaderType::eFragment;
}

graphics::ShaderType shaderTypeFromExt(io::Path const& extension) {
	auto const ext = extension.string();
	if (ext == ".vert") {
		return graphics::ShaderType::eVertex;
	} else if (ext == ".comp") {
		return graphics::ShaderType::eCompute;
	}
	return graphics::ShaderType::eFragment;
}
} // namespace

std::size_t AssetManifest::addSpirV(Group group) {
	std::size_t ret{};
	for (auto& [uri, json] : group) {
		AssetLoadData<graphics::SpirV> data;
		if (auto type = json->get_as<std::string_view>("type"); !type.empty()) {
			data.type = parseShaderType(type);
		} else {
			data.type = shaderTypeFromExt(io::Path(uri).extension());
		}
		data.uri = uri;
		m_spirV.add(std::move(uri), std::move(data));
		++ret;
	}
	return ret;
}

std::size_t AssetManifest::addTextures(Group group) {
	std::size_t ret{};
	for (auto& [uri, json] : group) {
		using Payload = graphics::Texture::Payload;
		AssetLoadData<graphics::Texture> data(&vram());
		if (auto files = json->find_as<std::vector<std::string>>("files")) {
			data.imageURIs = {files->begin(), files->end()};
		} else if (auto file = json->find_as<std::string>("file")) {
			data.imageURIs = {std::move(*file)};
		} else {
			data.imageURIs = {uri};
		}
		data.prefix = json->get_as<std::string>("prefix");
		data.ext = json->get_as<std::string>("ext");
		data.samplerURI = json->get_as<std::string_view>("sampler");
		if (auto payload = json->find_as<std::string_view>("payload")) { data.payload = *payload == "data" ? Payload::eData : Payload::eColour; }
		m_textures.add(std::move(uri), std::move(data));
		++ret;
	}
	return ret;
}

std::size_t AssetManifest::addPipelineStates(Group group) {
	std::size_t ret{};
	for (auto& [uri, json] : group) {
		if (auto shaders = json->find("shaders"); shaders && shaders->is_array()) {
			AssetLoadData<PipelineState> data;
			for (auto const& uri : shaders->as<dj::vec_t>()) {
				if (uri->is_string()) { data.shader.moduleURIs.push_back(uri->as<std::string>()); }
			}
			data.lineWidth = json->get_as("line_width", 1.0f);
			if (auto flags = json->find_as<std::vector<std::string>>("flags")) { data.flags = parseFlags(*flags); }
			if (auto pm = json->find_as<std::string_view>("polygon_mode")) { data.polygonMode = parsePolygonMode(*pm); }
			if (auto topology = json->find_as<std::string_view>("topology")) { data.topology = parseTopology(*topology); }
			m_pipelineStates.add(std::move(uri), std::move(data));
			++ret;
		}
	}
	return ret;
}

std::size_t AssetManifest::addRenderLayers(Group group) {
	std::size_t ret{};
	for (auto& [uri, json] : group) {
		if (auto const pipe = json->find("pipeline"); pipe && pipe->is_string()) {
			Hash const pid = pipe->as<std::string_view>();
			s64 const order = json->get_as<s64>("order");
			m_renderLayers.add(std::move(uri), [this, pid, order]() {
				return std::make_unique<RenderLayer>(RenderLayer{store().find<PipelineState>(pid).peek(), order});
			});
			++ret;
		}
	}
	return ret;
}

std::size_t AssetManifest::addBitmapFonts(Group group) {
	std::size_t ret{};
	for (auto& [uri, json] : group) {
		AssetLoadData<graphics::BitmapFont> data(&vram());
		if (auto file = json->find("file"); file && file->is_string()) {
			data.jsonURI = file->as<std::string_view>();
		} else {
			io::Path path(uri);
			data.jsonURI = path / path.filename() + ".json";
		}
		data.samplerURI = json->get_as<std::string_view>("sampler");
		m_bitmapFonts.add(std::move(uri), std::move(data));
		++ret;
	}
	return ret;
}

namespace {
graphics::RGBA parseRGBA(dj::json const& json, graphics::RGBA fallback) {
	if (json.is_string()) {
		if (auto str = json.as<std::string_view>(); !str.empty() && str[0] == '#') { return Colour(str); }
	} else if (json.contains("colour")) {
		if (auto str = json.get_as<std::string_view>("colour"); !str.empty() && str[0] == '#') {
			graphics::RGBA ret;
			ret.colour = Colour(str);
			ret.type = json.get_as<std::string_view>("type") == "absolute" ? graphics::RGBA::Type::eAbsolute : graphics::RGBA::Type::eIntensity;
			return ret;
		}
	}
	return fallback;
}
} // namespace

std::size_t AssetManifest::addMaterials(Group group) {
	std::size_t ret{};
	for (auto& [uri, json] : group) {
		Material mat;
		std::array<Hash, 4> texURIs;
		texURIs[0] = json->get_as<std::string_view>("map_Kd");
		texURIs[1] = json->get_as<std::string_view>("map_Ks");
		texURIs[2] = json->get_as<std::string_view>("map_d");
		texURIs[3] = json->get_as<std::string_view>("map_bump");
		mat.Ka = parseRGBA(json->get("Ka"), mat.Ka);
		mat.Kd = parseRGBA(json->get("Kd"), mat.Kd);
		mat.Ks = parseRGBA(json->get("Ks"), mat.Ks);
		mat.Tf = parseRGBA(json->get("Tf"), mat.Tf);
		mat.Ns = json->get_as<f32>("Ns", mat.Ns);
		mat.d = json->get_as<f32>("d", mat.d);
		mat.illum = json->get_as<int>("illum", mat.illum);
		m_materials.add(std::move(uri), [this, mat, texURIs]() {
			auto ret = std::make_unique<Material>(mat);
			ret->map_Kd = store().find<graphics::Texture>(texURIs[0]).peek();
			ret->map_Ks = store().find<graphics::Texture>(texURIs[1]).peek();
			ret->map_d = store().find<graphics::Texture>(texURIs[2]).peek();
			ret->map_Bump = store().find<graphics::Texture>(texURIs[3]).peek();
			return ret;
		});
		++ret;
	}
	return ret;
}

std::size_t AssetManifest::addSkyboxes(Group group) {
	std::size_t ret{};
	for (auto& [uri, json] : group) {
		if (auto const cubemap = json->find("cubemap"); cubemap && cubemap->is_string()) {
			m_skyboxes.add(uri, [this, cb = cubemap->as<std::string>(), u = std::move(uri)]() -> std::unique_ptr<Skybox> {
				if (auto cube = store().find<Cubemap>(cb)) { return std::make_unique<Skybox>(&*cube); }
				utils::g_log.log(dl::level::warn, 1, "[Asset] Failed to find Cubemap [{}] for Skybox [{}]", cb, u);
				return {};
			});
			++ret;
		}
	}
	return ret;
}

std::size_t AssetManifest::addModels(Group group) {
	std::size_t ret{};
	for (auto& [uri, json] : group) {
		AssetLoadData<Model> data(&vram());
		data.modelURI = uri;
		if (auto file = json->find("file"); file && file->is_string()) {
			data.jsonURI = file->as<std::string_view>();
		} else {
			io::Path path(uri);
			data.jsonURI = path / path.filename() + ".json";
		}
		data.samplerURI = json->get_as<std::string_view>("sampler");
		m_models.add(std::move(uri), std::move(data));
		++ret;
	}
	return ret;
}

std::size_t AssetManifest::unload() {
	static constexpr auto start_ = __LINE__;
	std::size_t ret = unloadMap(m_samplers.map);
	ret += unloadMap(m_spirV.map);
	ret += unloadMap(m_textures.map);
	ret += unloadMap(m_models.map);
	ret += unloadMap(m_materials.map);
	ret += unloadMap(m_bitmapFonts.map);
	ret += unloadMap(m_skyboxes.map);
	ret += unloadMap(m_pipelineStates.map);
	ret += unloadMap(m_renderLayers.map);
	static_assert(__LINE__ - start_ - 1 == int(Kind::eCOUNT_));
	ret += unloadCustom();
	return ret;
}
} // namespace le
