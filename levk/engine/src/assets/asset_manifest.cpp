#include <ktl/enumerate.hpp>
#include <levk/engine/assets/asset_converters.hpp>
#include <levk/engine/assets/asset_manifest.hpp>
#include <levk/engine/assets/asset_monitor.hpp>
#include <levk/engine/render/layer.hpp>
#include <levk/engine/render/pipeline.hpp>
#include <levk/graphics/font/font.hpp>
#include <levk/graphics/mesh.hpp>
#include <levk/graphics/render/context.hpp>
#include <levk/graphics/skybox.hpp>
#include <levk/graphics/texture.hpp>
#include <levk/graphics/utils/utils.hpp>

namespace le {
namespace {
bool isGlsl(io::Path const& path) { return path.has_extension() && (path.extension() == ".vert" || path.extension() == ".frag"); }

template <bool D = levk_debug>
io::Path spirvPath(io::Path const& glsl, io::FSMedia const& media);

template <>
[[maybe_unused]] io::Path spirvPath<true>(io::Path const& glsl, io::FSMedia const& media) {
	auto dbg = graphics::utils::spirVpath(glsl);
	if (auto res = graphics::utils::compileGlsl(media.fullPath(glsl))) {
		dbg = *res;
	} else {
		if (media.present(dbg)) {
			logW(LC_LibUser, "[Assets] Shader compilation failed, using existing SPIR-V [{}]", dbg.generic_string());
			return dbg;
		}
		if (auto rel = graphics::utils::spirVpath(glsl, false); media.present(rel)) {
			logW(LC_LibUser, "[Assets] Shader compilation failed, using existing SPIR-V [{}]", rel.generic_string());
			return rel;
		}
		ENSURE(false, "Failed to compile GLSL");
	}
	// compile Release shader too
	graphics::utils::compileGlsl(media.fullPath(glsl), {}, {}, false);
	return dbg;
}

template <>
[[maybe_unused]] io::Path spirvPath<false>(io::Path const& glsl, io::FSMedia const& media) {
	auto spv = graphics::utils::spirVpath(glsl);
	if (!media.present(spv, std::nullopt)) {
		if (auto res = graphics::utils::compileGlsl(media.fullPath(glsl))) { spv = *res; }
	}
	return spv;
}
} // namespace

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

ktl::kfunction<void()> spirVFunc(std::string uri, Engine::Service engine, dj::ptr<dj::json> const& json) {
	graphics::ShaderType shaderType{};
	if (auto type = json->get_as<std::string_view>("type"); !type.empty()) {
		shaderType = parseShaderType(type);
	} else {
		shaderType = shaderTypeFromExt(io::Path(uri).extension());
	}
	return [uri = std::move(uri), engine, shaderType]() mutable {
		Opt<io::FSMedia const> fsMedia{};
		io::Path path = uri;
		if (isGlsl(uri)) {
			if (fsMedia = dynamic_cast<io::FSMedia const*>(&engine.store().media()); fsMedia) {
				path = spirvPath(path, *fsMedia);
			} else {
				// cannot compile shaders without FSMedia
				path = graphics::utils::spirVpath(path);
			}
		} else {
			// fallback to previously compiled shader
			path = graphics::utils::spirVpath(path);
		}
		auto res = engine.store().media().bytes(path);
		if (!res) { return; }
		graphics::SpirV spirV;
		spirV.spirV = std::vector<u32>(res->size() / 4);
		spirV.type = shaderType;
		std::memcpy(spirV.spirV.data(), res->data(), res->size());
		engine.store().add(uri, std::move(spirV));
		if (ManifestLoader::s_attachMonitors && fsMedia) {
			engine.monitor().attach<graphics::SpirV>(uri, fsMedia->fullPath(uri), [uri, engine](graphics::SpirV& out) {
				if (auto fsMedia = dynamic_cast<io::FSMedia const*>(&engine.store().media())) {
					auto path = spirvPath(uri, *fsMedia);
					if (auto res = fsMedia->bytes(path)) {
						out.spirV = std::vector<u32>(res->size() / 4);
						std::memcpy(out.spirV.data(), res->data(), res->size());
						engine.context().pipelineFactory().markStale(uri);
						return true;
					}
				}
				return false;
			});
		}
	};
}

bool constructCubemap(io::Path const& prefix, Span<std::string const> files, io::Media const& media, graphics::Texture& out) {
	EXPECT(files.size() == 6U);
	bytearray bytes[6];
	ImageData cube[6];
	for (auto const& [file, idx] : ktl::enumerate(files)) {
		auto res = media.bytes(prefix / file);
		if (!res) { return false; }
		bytes[idx] = std::move(*res);
		cube[idx] = bytes[idx];
	}
	return out.construct(cube);
}

ktl::kfunction<void()> textureFunc(Engine::Service engine, std::string uri, dj::ptr<dj::json> const& json) {
	std::vector<std::string> files = json->get_as<std::vector<std::string>>("files");
	if (files.empty()) {
		if (auto file = json->find_as<std::string>("file")) {
			files.push_back(std::move(*file));
		} else {
			files.push_back(uri);
		}
	}
	Hash samplerURI = json->get_as<std::string>("sampler", "samplers/default");
	io::Path prefix = json->get_as<std::string>("prefix");
	return [uri, samplerURI, prefix, files, engine]() {
		auto sampler = engine.store().find<graphics::Sampler>(samplerURI);
		if (!sampler) { return; }
		graphics::Texture texture(&engine.vram(), sampler->sampler());
		if (files.size() > 1U) {
			if (constructCubemap(prefix, files, engine.store().media(), texture)) {
				engine.store().add(std::move(uri), std::move(texture));
				if (ManifestLoader::s_attachMonitors) {
					if (auto fsMedia = dynamic_cast<io::FSMedia const*>(&engine.store().media())) {
						io::Path paths[6];
						for (auto const& [file, idx] : ktl::enumerate(files)) { paths[idx] = fsMedia->fullPath(file); }
						auto f = [engine, files = std::move(files), prefix](graphics::Texture& out) {
							return constructCubemap(prefix, files, engine.store().media(), out);
						};
						engine.monitor().attach<graphics::Texture>(uri, paths, f);
					}
				}
			}
		} else {
			auto res = engine.store().media().bytes(files[0]);
			if (!res) { return; }
			if (texture.construct(*res)) {
				engine.store().add(uri, std::move(texture));
				if (ManifestLoader::s_attachMonitors) {
					if (auto fsMedia = dynamic_cast<io::FSMedia const*>(&engine.store().media())) {
						auto f = [engine, file = files[0]](graphics::Texture& out) {
							if (auto res = engine.store().media().bytes(file)) { return out.construct(*res); }
							return false;
						};
						engine.monitor().attach<graphics::Texture>(uri, fsMedia->fullPath(files[0]), f);
					}
				}
			}
		}
	};
}

ktl::kfunction<void()> fontFunc(Engine::Service engine, std::string uri, dj::ptr<dj::json> const& json) {
	io::Path ttfURI;
	if (auto file = json->find("file"); file && file->is_string()) {
		ttfURI = file->as<std::string_view>();
	} else {
		io::Path path(uri);
		ttfURI = path / path.filename() + ".ttf";
	}
	bool mipMaps = json->get_as<bool>("mip_maps", true);
	auto height = graphics::Font::Height{json->get_as<u32>("height", u32(graphics::Font::Height::eDefault))};
	return [uri, ttfURI, mipMaps, height, engine] {
		auto ttf = engine.store().media().bytes(ttfURI);
		if (!ttf) { return; }
		graphics::Font::Info fi;
		fi.name = ttfURI.filename().string();
		fi.ttf = *ttf;
		fi.height = height;
		fi.atlas.mipMaps = mipMaps;
		engine.store().add(std::move(uri), graphics::Font(&engine.vram(), std::move(fi)));
	};
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
		e.store().add(std::move(uri), rp);
	};
}

static graphics::RGBA parseRGBA(dj::json const& json, graphics::RGBA fallback) {
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

ktl::kfunction<void()> bpMaterialFunc(Engine::Service engine, std::string uri, dj::ptr<dj::json> const& json) {
	graphics::BPMaterialData mat;
	mat.Ka = parseRGBA(json->get("Ka"), mat.Ka);
	mat.Kd = parseRGBA(json->get("Kd"), mat.Kd);
	mat.Ks = parseRGBA(json->get("Ks"), mat.Ks);
	mat.Tf = parseRGBA(json->get("Tf"), mat.Tf);
	mat.Ns = json->get_as<f32>("Ns", mat.Ns);
	mat.d = json->get_as<f32>("d", mat.d);
	mat.illum = json->get_as<int>("illum", mat.illum);
	return [uri = std::move(uri), mat, engine] { engine.store().add(std::move(uri), mat); };
}

ktl::kfunction<void()> pbrMaterialFunc(Engine::Service engine, std::string uri, dj::ptr<dj::json> const& json) {
	graphics::PBRMaterialData mat;
	mat.alphaCutoff = json->get_as<f32>("alpha_cutoff", mat.alphaCutoff);
	mat.baseColourFactor = parseRGBA(json->get("base_colour_factor"), mat.baseColourFactor);
	mat.emissiveFactor = parseRGBA(json->get("emissive_factor"), mat.emissiveFactor);
	mat.metallicFactor = json->get_as<f32>("metallic_factor", mat.metallicFactor);
	mat.roughnessFactor = json->get_as<f32>("roughness_factor", mat.roughnessFactor);
	if (auto mode = json->find_as<std::string_view>("mode")) {
		if (*mode == "blend") {
			mat.mode = graphics::PBRMaterialData::Mode::eBlend;
		} else if (*mode == "mask") {
			mat.mode = graphics::PBRMaterialData::Mode::eMask;
		}
	}
	return [uri = std::move(uri), mat, engine] { engine.store().add(std::move(uri), mat); };
}

ktl::kfunction<void()> textureRefsFunc(Engine::Service engine, std::string uri, dj::ptr<dj::json> const& json) {
	TextureRefs texRefs;
	texRefs.textures[graphics::MatTexType::eDiffuse] = json->get_as<std::string_view>("diffuse");
	texRefs.textures[graphics::MatTexType::eSpecular] = json->get_as<std::string_view>("specular");
	texRefs.textures[graphics::MatTexType::eAlpha] = json->get_as<std::string_view>("alpha");
	texRefs.textures[graphics::MatTexType::eBump] = json->get_as<std::string_view>("bump");
	texRefs.textures[graphics::MatTexType::eMetalRough] = json->get_as<std::string_view>("metal_rough");
	texRefs.textures[graphics::MatTexType::eOcclusion] = json->get_as<std::string_view>("occlusion");
	texRefs.textures[graphics::MatTexType::eNormal] = json->get_as<std::string_view>("normal");
	texRefs.textures[graphics::MatTexType::eEmissive] = json->get_as<std::string_view>("emissive");
	return [uri = std::move(uri), texRefs, engine] { engine.store().add(std::move(uri), texRefs); };
}

ktl::kfunction<void()> skyboxFunc(Engine::Service engine, std::string uri, std::string cubemap) {
	return [engine, cb = std::move(cubemap), uri = std::move(uri)] {
		if (auto cube = engine.store().find<graphics::Texture>(cb); cube && cube->type() == graphics::Texture::Type::eCube) {
			engine.store().add(std::move(uri), graphics::Skybox(&engine.vram(), cube));
		} else {
			logW(LC_LibUser, "[Asset] Failed to find cubemap Texture [{}] for Skybox [{}]", cb, uri);
		}
	};
}

ktl::kfunction<void()> objMeshFunc(Engine::Service engine, std::string uri, dj::ptr<dj::json> const& json) {
	std::string meshJSON = json->get_as<std::string>("file");
	if (meshJSON.empty()) {
		io::Path path = uri;
		path /= path.filename();
		path += ".json";
		meshJSON = path.generic_string();
	}
	return [engine, json = std::move(meshJSON), uri = std::move(uri)] {
		if (auto mesh = graphics::Mesh::fromObjMtl(json, engine.store().media(), &engine.vram())) {
			engine.store().add(std::move(uri), std::move(*mesh));
		} else {
			logW(LC_LibUser, "[Asset] Failed to load Mesh from OBJ [{}]", json);
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
		if (name == "texture_refs") { return textureRefs(group); }
		if (name == "fonts") { return fonts(group); }
		if (name == "skyboxes") { return skyboxes(group); }
		if (name == "meshes") { return meshes(group); }
		return std::nullopt;
	}

	std::size_t samplers(Group const& group) const {
		using namespace graphics;
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			if (json->contains("min") && json->contains("mag")) {
				add(order<Sampler>(), std::move(uri), Sampler(&m_engine.device(), samplerInfo(json)));
				++ret;
			}
		}
		return ret;
	}

	std::size_t spirV(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			enqueue(order<graphics::SpirV>(), spirVFunc(std::move(uri), m_engine, json));
			++ret;
		}
		return ret;
	}

	std::size_t renderLayers(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			auto const rs = io::fromJson<RenderLayer>(*json);
			add(order<RenderLayer>(), std::move(uri), rs);
			++ret;
		}
		return ret;
	}

	std::size_t textures(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			enqueue(order<graphics::Texture>(), textureFunc(m_engine, std::move(uri), json));
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
			if (json->get_as<std::string_view>("type") == "pbr") {
				enqueue(order<graphics::PBRMaterialData>(), pbrMaterialFunc(m_engine, std::move(uri), json));
			} else {
				enqueue(order<graphics::BPMaterialData>(), bpMaterialFunc(m_engine, std::move(uri), json));
			}
			++ret;
		}
		return ret;
	}

	std::size_t textureRefs(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			enqueue(order<TextureRefs>(), textureRefsFunc(m_engine, std::move(uri), json));
			++ret;
		}
		return ret;
	}

	std::size_t fonts(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			enqueue(order<graphics::Font>(), fontFunc(m_engine, std::move(uri), json));
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

	std::size_t meshes(Group const& group) const {
		std::size_t ret{};
		for (auto& [uri, json] : group) {
			enqueue(order<graphics::Mesh>(), objMeshFunc(m_engine, std::move(uri), json));
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

void AssetManifest::Parser::enqueue(Order order, dts::task_t task) const { (*m_stages)[order].push_back(std::move(task)); }

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
	m_start = time::now();
	dts::future_t previous;
	for (auto& [_, stage] : m_stages) {
		std::span<dts::future_t> deps;
		if (previous.valid()) { deps = std::span<dts::future_t>(&previous, 1U); }
		previous = m_engine.executor().schedule(stage, deps);
	}
	auto const deps = std::span(&previous, 1U);
	dts::task_t finalTask = [e = m_engine, m = m_manifest, s = m_start] {
		if (auto const l = loaded(e, m); l > 0U) {
			auto const dt = time::diff(s);
			logI(LC_EndUser, "[Asset] [{}] Assets loaded in [{:.2f}s]", l, dt.count());
		}
	};
	auto const tasks = std::span(&finalTask, 1U);
	m_future = m_engine.executor().schedule(tasks, deps);
}

void ManifestLoader::loadBlocking() {
	m_start = time::now();
	for (auto const& [_, stage] : m_stages) {
		for (auto const& func : stage) { func(); }
	}
	if (auto const l = loaded(m_engine, m_manifest); l > 0U) {
		auto const dt = time::diff(m_start);
		logI(LC_EndUser, "[Asset] [{}] Assets loaded in [{:.2f}s]", l, dt.count());
	}
}

void ManifestLoader::load(io::Path const& jsonURI, Opt<Parser> custom, bool async, bool reload) {
	if (reload || m_manifest.list.empty()) {
		if (auto json = m_engine.store().media().string(jsonURI)) {
			dj::json root;
			if (root.read(*json)) { preload(root, custom); }
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
