#include <levk/engine/assets/asset_converters.hpp>
#include <levk/engine/assets/asset_manifest.hpp>
#include <levk/engine/render/pipeline.hpp>
#include <levk/engine/render/skybox.hpp>

namespace le {
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
	if (!ext.empty() && ext[0] == '.') { return parseShaderType(ext.substr(1)); }
	return graphics::ShaderType::eFragment;
}

vk::SamplerCreateInfo samplerInfo(dj::ptr<dj::json> const& json) {
	graphics::TPair<vk::Filter> minMag;
	minMag.first = (*json)["min"].as<std::string_view>() == "nearest" ? vk::Filter::eNearest : vk::Filter::eLinear;
	minMag.second = (*json)["mag"].as<std::string_view>() == "nearest" ? vk::Filter::eNearest : vk::Filter::eLinear;
	vk::SamplerCreateInfo ret;
	if (auto mipMode = json->find_as<std::string_view>("mip_mode")) {
		auto const mm = *mipMode == "nearest" ? vk::SamplerMipmapMode::eNearest : vk::SamplerMipmapMode::eLinear;
		ret = graphics::Sampler::info(minMag, mm);
	} else {
		ret = graphics::Sampler::info(minMag);
	}
	return ret;
}

AssetLoadData<graphics::SpirV> spirVData(std::string_view uri, dj::ptr<dj::json> const& json) {
	AssetLoadData<graphics::SpirV> data;
	if (auto type = json->get_as<std::string_view>("type"); !type.empty()) {
		data.type = parseShaderType(type);
	} else {
		data.type = shaderTypeFromExt(io::Path(uri).extension());
	}
	data.uri = uri;
	return data;
}

AssetLoadData<graphics::Texture> textureData(graphics::VRAM& vram, std::string_view uri, dj::ptr<dj::json> const& json) {
	using Payload = graphics::Texture::Payload;
	AssetLoadData<graphics::Texture> ret(&vram);
	if (auto files = json->find_as<std::vector<std::string>>("files")) {
		ret.imageURIs = {files->begin(), files->end()};
	} else if (auto file = json->find_as<std::string>("file")) {
		ret.imageURIs = {std::move(*file)};
	} else {
		ret.imageURIs = {uri};
	}
	ret.prefix = json->get_as<std::string>("prefix");
	ret.ext = json->get_as<std::string>("ext");
	ret.samplerURI = json->get_as<std::string_view>("sampler");
	if (auto payload = json->find_as<std::string_view>("payload")) { ret.payload = *payload == "data" ? Payload::eData : Payload::eColour; }
	return ret;
}

AssetLoadData<graphics::Font> fontData(graphics::VRAM& vram, std::string_view uri, dj::ptr<dj::json> const& json) {
	AssetLoadData<graphics::Font> ret(&vram);
	if (auto file = json->find("file"); file && file->is_string()) {
		ret.ttfURI = file->as<std::string_view>();
	} else {
		io::Path path(uri);
		ret.ttfURI = path / path.filename() + ".ttf";
	}
	ret.info.atlas.mipMaps = json->get_as<bool>("mip_maps", ret.info.atlas.mipMaps);
	ret.info.height = graphics::Font::Height{json->get_as<u32>("height", u32(ret.info.height))};
	return ret;
}

AssetLoadData<Model> modelData(graphics::VRAM& vram, std::string_view uri, dj::ptr<dj::json> const& json) {
	AssetLoadData<Model> data(&vram);
	data.modelURI = uri;
	if (auto file = json->find("file"); file && file->is_string()) {
		data.jsonURI = file->as<std::string_view>();
	} else {
		io::Path path(uri);
		data.jsonURI = path / path.filename() + ".json";
	}
	data.samplerURI = json->get_as<std::string_view>("sampler");
	return data;
}

ktl::kfunction<void()> renderPipelineFunc(Engine::Service engine, std::string uri, std::string layer, std::vector<std::string> shaders) {
	return [e = engine, uri = std::move(uri), st = std::move(layer), sh = std::move(shaders)]() {
		auto layer = e.store().find<RenderLayer>(st);
		if (!layer) {
			logW(LC_LibUser, "[Asset] Failed to find RenderLayer [{}]", st);
			return;
		}
		RenderPipeline rp;
		rp.layer = *layer;
		rp.shaderURIs = {sh.begin(), sh.end()};
		e.store().add(std::move(uri), std::make_unique<RenderPipeline>(rp));
	};
}

ktl::kfunction<void()> materialsFunc(Engine::Service engine, std::string uri, dj::ptr<dj::json> const& json) {
	Material mat = io::fromJson<Material>(*json);
	return [uri = std::move(uri), engine, mat]() { engine.store().add(std::move(uri), mat); };
}

ktl::kfunction<void()> skyboxFunc(Engine::Service engine, std::string uri, std::string cubemap) {
	return [engine, cb = std::move(cubemap), uri = std::move(uri)] {
		if (auto cube = engine.store().find<graphics::Texture>(cb); cube && cube->type() == graphics::Texture::Type::eCube) {
			engine.store().add(std::move(uri), std::make_unique<Skybox>(&*cube));
		} else {
			logW(LC_LibUser, "[Asset] Failed to find cubemap Texture [{}] for Skybox [{}]", cb, uri);
		}
	};
}

struct DefaultParser : AssetManifest::Parser {
	using Parser::Parser;

	std::optional<std::size_t> operator()(std::string_view name, Group const& group) const override {
		if (name == "samplers") { return samplers(group); }
		if (name == "shaders") { return spirV(group); }
		if (name == "render_layers") { return renderLayers(group); }
		if (name == "textures") { return textures(group); }
		if (name == "render_pipelines") { return renderPipelines(group); }
		if (name == "materials") { return materials(group); }
		if (name == "fonts") { return fonts(group); }
		if (name == "skyboxes") { return skyboxes(group); }
		if (name == "models") { return models(group); }
		return std::nullopt;
	}

	std::size_t samplers(Group const& group) const {
		using namespace graphics;
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			if (json->contains("min") && json->contains("mag")) {
				add(order<Sampler>(), std::move(uri), std::make_unique<Sampler>(&m_engine.device(), samplerInfo(json)));
				++ret;
			}
		}
		return ret;
	}

	std::size_t spirV(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			auto data = spirVData(uri, json);
			load(order<graphics::SpirV>(), std::move(uri), std::move(data));
			++ret;
		}
		return ret;
	}

	std::size_t renderLayers(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			auto const rs = io::fromJson<RenderLayer>(*json);
			add(order<RenderLayer>(), std::move(uri), std::make_unique<RenderLayer>(rs));
			++ret;
		}
		return ret;
	}

	std::size_t textures(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			auto data = textureData(m_engine.vram(), uri, json);
			load(order<graphics::Texture>(), std::move(uri), std::move(data));
			++ret;
		}
		return ret;
	}

	std::size_t renderPipelines(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			auto layer = json->get_as<std::string>("layer");
			auto shaders = json->get_as<std::vector<std::string>>("shaders");
			if (!layer.empty() && !shaders.empty()) {
				enqueue(order<RenderPipeline>(), renderPipelineFunc(m_engine, std::move(uri), std::move(layer), std::move(shaders)));
				++ret;
			}
		}
		return ret;
	}

	std::size_t materials(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			enqueue(order<Material>(), materialsFunc(m_engine, std::move(uri), json));
			++ret;
		}
		return ret;
	}

	std::size_t fonts(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			auto data = fontData(m_engine.vram(), uri, json);
			load(order<graphics::Font>(), std::move(uri), std::move(data));
			++ret;
		}
		return ret;
	}

	std::size_t skyboxes(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			if (auto cubemap = json->find_as<std::string>("cubemap")) {
				enqueue(depend<graphics::Texture>(), skyboxFunc(m_engine, std::move(uri), std::move(*cubemap)));
				++ret;
			}
		}
		return ret;
	}

	std::size_t models(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			auto data = modelData(m_engine.vram(), uri, json);
			load(order<Model>(), std::move(uri), std::move(data));
			++ret;
		}
		return ret;
	}
};

std::size_t loaded(Engine::Service engine, AssetManifest const& manifest) {
	std::size_t ret{};
	for (auto const& [_, group] : manifest.list) {
		for (auto const& [uri, _] : group) {
			if (engine.store().exists(uri)) { ++ret; }
		}
	}
	return ret;
}
} // namespace

AssetManifest& AssetManifest::include(List add) {
	for (auto& [uri, group] : add) { list.insert_or_assign(std::move(uri), std::move(group)); }
	return *this;
}

AssetManifest& AssetManifest::exclude(List const& remove) {
	for (auto const& [uri, _] : remove) { list.erase(uri); }
	return *this;
}

AssetManifest::List AssetManifest::populate(dj::json const& root) {
	List ret;
	for (auto& [name, entries] : root.as<dj::map_t>()) {
		Group group;
		for (auto& json : entries->as<dj::vec_t>()) {
			if (auto uri = json->find_as<std::string>("uri")) { group.insert({std::move(*uri), std::move(json)}); }
			if (group.empty()) {
				for (auto& uri : entries->as<std::vector<std::string>>()) { group.insert({std::move(uri), std::make_shared<dj::json>()}); }
			}
		}
		if (!group.empty()) { ret.emplace(std::move(name), std::move(group)); }
	}
	return ret;
}

static std::size_t recurse(std::string_view name, AssetManifest::Group const& group, AssetManifest::Parser const& parser) {
	if (auto ret = parser(name, group)) { return *ret; }
	if (parser.m_next) { return recurse(name, group, *parser.m_next); }
	return {};
}

std::size_t ManifestLoader::preload(dj::json const& root, Opt<Parser const> custom) {
	m_manifest.list = AssetManifest::populate(root);
	DefaultParser parser(m_engine, &m_stages, custom);
	std::size_t ret = {};
	for (auto const& [id, group] : m_manifest.list) { ret += recurse(id, group, parser); }
	if (ret > 0U) { logI(LC_EndUser, "[Asset] [{}] Assets preloaded into manifest", ret); }
	return ret;
}

void ManifestLoader::loadAsync() {
	if (m_stages.empty()) { return; }
	dts::future_t previous;
	for (auto& [_, stage] : m_stages) {
		std::span<dts::future_t> deps;
		if (previous.valid()) { deps = std::span<dts::future_t>(&previous, 1U); }
		previous = m_engine.executor().schedule(stage, deps);
	}
	auto const deps = std::span(&previous, 1U);
	dts::task_t finalTask = [e = m_engine, m = m_manifest] {
		if (auto const l = loaded(e, m); l > 0U) { logI(LC_EndUser, "[Asset] [{}] Assets loaded", l); }
	};
	auto const tasks = std::span(&finalTask, 1U);
	m_future = m_engine.executor().schedule(tasks, deps);
}

void ManifestLoader::loadBlocking() {
	for (auto const& [_, stage] : m_stages) {
		for (auto const& func : stage) { func(); }
	}
	if (auto const l = loaded(m_engine, m_manifest); l > 0U) { logI(LC_EndUser, "[Asset] [{}] Assets loaded", l); }
}

void ManifestLoader::load(io::Path jsonURI, Opt<Parser> custom, bool async, bool reload) {
	if (reload || m_manifest.list.empty()) {
		if (auto json = m_engine.store().resources().load(std::move(jsonURI), Resource::Type::eText, Resources::Flag::eReload)) {
			dj::json root;
			if (root.read(json->string())) { preload(root, custom); }
		}
	}
	if (async) {
		loadAsync();
	} else {
		loadBlocking();
	}
}

std::size_t ManifestLoader::unload() {
	std::size_t ret{};
	for (auto const& [_, group] : m_manifest.list) {
		for (auto const& [id, _] : group) {
			if (m_engine.store().unload(id)) { ++ret; }
		}
	}
	if (ret > 0U) { logI(LC_EndUser, "[Asset] [{}] Assets unloaded", ret); }
	return ret;
}

void ManifestLoader::clear() {
	wait();
	m_manifest.list.clear();
	m_stages.clear();
	m_future = {};
}

void ManifestLoader::wait() const {
	if (m_future.valid()) { m_future.wait(); }
}
} // namespace le
