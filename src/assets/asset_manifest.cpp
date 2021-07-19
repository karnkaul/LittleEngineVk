#include <engine/assets/asset_manifest.hpp>
#include <engine/engine.hpp>
#include <graphics/context/bootstrap.hpp>
#include <kt/enum_flags/enumerate_enum.hpp>

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
} // namespace

void AssetManifest::append(AssetManifest const& rhs) {
	m_samplers = m_samplers + rhs.m_samplers;
	m_textures = m_textures + rhs.m_textures;
}

std::size_t AssetManifest::preload(dj::json_t const& root) {
	std::size_t ret{};
	for (auto const& [groupName, entries] : root.as<dj::map_t>()) {
		Group group;
		for (auto const& json : entries->as<dj::vec_t>()) {
			if (auto id = json->find_as<std::string>("id")) { group.insert({std::move(*id), json}); }
		}
		if (!group.empty()) { ret += add(groupName, std::move(group)); }
	}
	return ret;
}

void AssetManifest::stage(dts::scheduler* scheduler) {
	m_deps[Kind::eSampler] = m_loader.stage(std::move(m_samplers), scheduler);
	m_deps[Kind::eTexture] = m_loader.stage(std::move(m_textures), scheduler, m_deps[Kind::eSampler]);
	m_deps[Kind::eShader] = m_loader.stage(std::move(m_shaders), scheduler);
	m_deps[Kind::ePipeline] = m_loader.stage(std::move(m_pipelines), scheduler, m_deps[Kind::eShader]);
	m_deps[Kind::eDrawLayer] = m_loader.stage(std::move(m_drawLayers), scheduler, m_deps[Kind::ePipeline]);
	m_deps[Kind::eBitmapFont] = m_loader.stage(std::move(m_bitmapFonts), scheduler, m_deps[Kind::eSampler]);
	m_deps[Kind::eModel] = m_loader.stage(std::move(m_models), scheduler);
	loadCustom(scheduler);
}

std::size_t AssetManifest::load(io::Path const& jsonID, dts::scheduler* scheduler) {
	std::size_t ret{};
	if (auto eng = Services::locate<Engine>()) {
		dj::json_t json;
		auto& resources = eng->store().resources();
		auto res = resources.load(jsonID, Resource::Type::eText);
		if (!res && !jsonID.has_extension()) { res = resources.load(jsonID + ".manifest", Resource::Type::eText); }
		if (res && json.read(res->string())) { ret = preload(json); }
	}
	if (ret > 0) { stage(scheduler); }
	return ret;
}

std::size_t AssetManifest::unload(io::Path const& jsonID, dts::scheduler& scheduler) {
	wait(scheduler);
	std::size_t count{};
	if (auto eng = Services::locate<Engine>()) {
		dj::json_t json;
		auto& resources = eng->store().resources();
		auto res = resources.load(jsonID, Resource::Type::eText);
		if (!res && !jsonID.has_extension()) { res = resources.load(jsonID + ".manifest", Resource::Type::eText); }
		if (res && json.read(res->string())) { count = preload(json); }
	}
	if (count > 0) { return unload(); }
	return 0;
}

std::vector<AssetManifest::StageID> AssetManifest::deps(Kinds kinds) const noexcept {
	std::vector<StageID> ret;
	ret.reserve(std::size_t(Kind::eCOUNT_));
	for (Kind const k : kt::enumerate_enum<Kind>()) {
		if (kinds.test(k)) { ret.push_back(m_deps[k]); }
	}
	return ret;
}

graphics::Device& AssetManifest::device() { return engine()->gfx().boot.device; }
graphics::VRAM& AssetManifest::vram() { return engine()->gfx().boot.vram; }
graphics::RenderContext& AssetManifest::context() { return engine()->gfx().context; }
AssetStore& AssetManifest::store() { return engine()->store(); }
not_null<Engine*> AssetManifest::engine() { return m_engine ? m_engine : (m_engine = Services::locate<Engine>()); }

std::size_t AssetManifest::add(std::string_view groupName, Group group) {
	if (groupName == "samplers") { return addSamplers(std::move(group)); }
	if (groupName == "shaders") { return addShaders(std::move(group)); }
	if (groupName == "textures") { return addTextures(std::move(group)); }
	if (groupName == "models") { return addModels(std::move(group)); }
	if (groupName == "bitmap_fonts") { return addBitmapFonts(std::move(group)); }
	if (groupName == "pipelines") { return addPipelines(std::move(group)); }
	if (groupName == "draw_layers") { return addDrawLayers(std::move(group)); }
	return addCustom(groupName, std::move(group));
}

std::size_t AssetManifest::addSamplers(Group group) {
	std::size_t ret{};
	auto& dv = device();
	for (auto& [id, json] : group) {
		if (json->contains("min") && json->contains("mag")) {
			graphics::TPair<vk::Filter> minMag;
			minMag.first = (*json)["min"].as<std::string>() == "nearest" ? vk::Filter::eNearest : vk::Filter::eLinear;
			minMag.second = (*json)["mag"].as<std::string>() == "nearest" ? vk::Filter::eNearest : vk::Filter::eLinear;
			vk::SamplerCreateInfo createInfo;
			if (auto mipMode = json->find_as<std::string>("mip_mode")) {
				auto const mm = *mipMode == "nearest" ? vk::SamplerMipmapMode::eNearest : vk::SamplerMipmapMode::eLinear;
				createInfo = graphics::Sampler::info(minMag, mm);
			} else {
				createInfo = graphics::Sampler::info(minMag);
			}
			m_samplers.add(std::move(id), [createInfo, &dv]() { return graphics::Sampler(&dv, createInfo); });
			++ret;
		}
	}
	return ret;
}

std::size_t AssetManifest::addShaders(Group group) {
	std::size_t ret{};
	for (auto& [id, json] : group) {
		if (auto files = json->find_as<std::vector<std::string>>("files")) {
			AssetLoadData<graphics::Shader> data(&device());
			data.name = id.generic_string();
			for (auto const& str : *files) {
				io::Path path = str;
				auto const ext = path.extension().generic_string();
				if (ext.starts_with(".vert")) {
					data.shaderPaths.emplace(graphics::Shader::Type::eVertex, std::move(path));
				} else if (ext.starts_with(".frag")) {
					data.shaderPaths.emplace(graphics::Shader::Type::eFragment, std::move(path));
				}
			}
			m_shaders.add(std::move(id), std::move(data));
			++ret;
		}
	}
	return ret;
}

std::size_t AssetManifest::addTextures(Group group) {
	std::size_t ret{};
	for (auto& [id, json] : group) {
		using Payload = graphics::Texture::Payload;
		AssetLoadData<graphics::Texture> data(&vram());
		if (auto files = json->find_as<std::vector<std::string>>("files")) {
			data.imageIDs = {files->begin(), files->end()};
		} else if (auto file = json->find_as<std::string>("file")) {
			data.imageIDs = {std::move(*file)};
		} else {
			data.imageIDs = {id.generic_string()};
		}
		data.prefix = json->get_as<std::string>("prefix");
		data.ext = json->get_as<std::string>("ext");
		data.samplerID = json->get_as<std::string>("sampler");
		if (auto payload = json->find_as<std::string>("payload")) { data.payload = *payload == "data" ? Payload::eData : Payload::eColour; }
		m_textures.add(std::move(id), std::move(data));
		++ret;
	}
	return ret;
}

std::size_t AssetManifest::addPipelines(Group group) {
	std::size_t ret{};
	for (auto& [id, json] : group) {
		if (auto shader = json->find("shader"); shader && shader->is_string()) {
			AssetLoadData<graphics::Pipeline> data(&context());
			data.name = id.generic_string();
			data.shaderID = shader->as<std::string>();
			if (auto name = json->find_as<std::string>("name")) { data.name = std::move(*name); }
			if (auto flags = json->find_as<std::vector<std::string>>("flags")) { data.flags = parseFlags(*flags); }
			data.gui = json->get_as<bool>("gui");
			data.wireframe = json->get_as<f32>("wireframe");
			m_pipelines.add(std::move(id), std::move(data));
			++ret;
		}
	}
	return ret;
}

std::size_t AssetManifest::addDrawLayers(Group group) {
	std::size_t ret{};
	for (auto& [id, json] : group) {
		if (auto const pipe = json->find("pipeline"); pipe && pipe->is_string()) {
			Hash const pid = pipe->as<std::string>();
			s64 const order = json->get_as<s64>("order");
			m_drawLayers.add(std::move(id), [this, pid, order]() { return DrawLayer{store().find<graphics::Pipeline>(pid).peek(), order}; });
			++ret;
		}
	}
	return ret;
}

std::size_t AssetManifest::addBitmapFonts(Group group) {
	std::size_t ret{};
	for (auto& [id, json] : group) {
		AssetLoadData<BitmapFont> data(&vram());
		if (auto file = json->find("file"); file && file->is_string()) {
			data.jsonID = file->as<std::string>();
		} else {
			data.jsonID = id / id.filename() + ".json";
		}
		data.samplerID = json->get_as<std::string>("sampler");
		m_bitmapFonts.add(std::move(id), std::move(data));
		++ret;
	}
	return ret;
}

std::size_t AssetManifest::addModels(Group group) {
	std::size_t ret{};
	for (auto& [id, json] : group) {
		AssetLoadData<Model> data(&vram());
		data.modelID = id.generic_string();
		if (auto file = json->find("file"); file && file->is_string()) {
			data.jsonID = file->as<std::string>();
		} else {
			data.jsonID = id / id.filename() + ".json";
		}
		data.samplerID = json->get_as<std::string>("sampler");
		m_models.add(std::move(id), std::move(data));
		++ret;
	}
	return ret;
}

template <typename T, typename U>
std::size_t AssetManifest::unload(U& cont) {
	std::size_t ret{};
	for (auto const& [id, _] : cont) {
		if (store().unload<T>(id)) { ++ret; }
	}
	cont.clear();
	return ret;
}

std::size_t AssetManifest::unload() {
	std::size_t ret = unload<graphics::Sampler>(m_samplers.map);
	ret += unload<graphics::Shader>(m_shaders.map);
	ret += unload<graphics::Texture>(m_textures.map);
	ret += unload<Model>(m_models.map);
	ret += unload<BitmapFont>(m_bitmapFonts.map);
	ret += unload<graphics::Pipeline>(m_pipelines.map);
	ret += unload<DrawLayer>(m_drawLayers.map);
	ret += unloadCustom();
	return ret;
}
} // namespace le
