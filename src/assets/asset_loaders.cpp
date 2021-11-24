#include <core/services.hpp>
#include <dumb_json/json.hpp>
#include <engine/assets/asset_converters.hpp>
#include <engine/assets/asset_loaders.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/utils/logger.hpp>
#include <graphics/render/pipeline_factory.hpp>
#include <graphics/utils/utils.hpp>
#include <algorithm>

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
			logW("[Assets] Shader compilation failed, using existing SPIR-V [{}]", dbg.generic_string());
			return dbg;
		}
		if (auto rel = graphics::utils::spirVpath(glsl, false); media.present(rel)) {
			logW("[Assets] Shader compilation failed, using existing SPIR-V [{}]", rel.generic_string());
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

std::unique_ptr<graphics::SpirV> AssetLoader<graphics::SpirV>::load(AssetLoadInfo<graphics::SpirV> const& info) const {
	graphics::SpirV ret;
	if (reload(ret, info)) { return std::make_unique<graphics::SpirV>(std::move(ret)); }
	return {};
}

bool AssetLoader<graphics::SpirV>::reload(graphics::SpirV& out_code, AssetLoadInfo<graphics::SpirV> const& info) const {
	auto const ret = load(out_code, info);
	if (ret) {
		if (auto context = Services::find<graphics::RenderContext>()) { context->pipelineFactory().markStale(info.m_uri); }
	}
	return ret;
}

bool AssetLoader<graphics::SpirV>::load(graphics::SpirV& out_code, AssetLoadInfo<graphics::SpirV> const& info) const {
	auto path = info.m_data.uri;
	if (isGlsl(path)) {
		if (auto fm = dynamic_cast<io::FSMedia const*>(&info.media())) {
			// ensure resource presence (and add monitor if supported)
			if (!info.resource(path, Resource::Type::eText, Resources::Flag::eMonitor)) { return false; }
			path = spirvPath(path, *fm);
		} else {
			// cannot compile shaders without FSMedia
			path = graphics::utils::spirVpath(path);
		}
	} else {
		// fallback to previously compiled shader
		path = graphics::utils::spirVpath(path);
	}
	auto res = info.resource(path, Resource::Type::eBinary, Resources::Flag::eReload);
	if (!res) { return false; }
	out_code.spirV = std::vector<u32>(res->bytes().size() / 4);
	out_code.type = info.m_data.type;
	std::memcpy(out_code.spirV.data(), res->bytes().data(), res->bytes().size());
	return true;
}

std::unique_ptr<PipelineState> AssetLoader<PipelineState>::load(AssetLoadInfo<PipelineState> const& info) const {
	for (Hash const uri : info.m_data.shader.moduleURIs) {
		if (!info.m_store->find<graphics::SpirV>(uri)) { return {}; }
	}
	return std::make_unique<PipelineState>(from(info.m_data));
}

bool AssetLoader<PipelineState>::reload(PipelineState& out_ps, AssetLoadInfo<PipelineState> const& info) const {
	for (Hash const uri : info.m_data.shader.moduleURIs) {
		if (!info.m_store->find<graphics::SpirV>(uri)) { return false; }
	}
	out_ps = from(info.m_data);
	return true;
}

PipelineState AssetLoader<PipelineState>::from(AssetLoadData<PipelineState> const& data) {
	PipelineState ret = graphics::PipelineFactory::spec(data.shader, data.flags);
	ret.fixedState.mode = data.polygonMode;
	ret.fixedState.lineWidth = data.lineWidth;
	ret.fixedState.topology = data.topology;
	return ret;
}

std::unique_ptr<graphics::Texture> AssetLoader<graphics::Texture>::load(AssetLoadInfo<graphics::Texture> const& info) const {
	auto const samplerURI = info.m_data.samplerURI == Hash{} ? "samplers/default" : info.m_data.samplerURI;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerURI);
	if (!sampler) { return {}; }
	if (auto d = data(info)) {
		graphics::Texture::CreateInfo createInfo;
		createInfo.data = std::move(*d);
		createInfo.sampler = sampler->sampler();
		createInfo.forceFormat = info.m_data.forceFormat;
		createInfo.payload = info.m_data.payload;
		graphics::Texture ret(info.m_data.vram);
		if (ret.construct(createInfo)) { return std::make_unique<graphics::Texture>(std::move(ret)); }
	}
	return {};
}

bool AssetLoader<graphics::Texture>::reload(graphics::Texture& out_texture, AssetLoadInfo<graphics::Texture> const& info) const {
	auto const samplerURI = info.m_data.samplerURI == Hash{} ? "samplers/default" : info.m_data.samplerURI;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerURI);
	if (!sampler) { return false; }
	if (auto d = data(info)) {
		graphics::Texture::CreateInfo createInfo;
		createInfo.data = std::move(*d);
		createInfo.forceFormat = info.m_data.forceFormat;
		createInfo.payload = info.m_data.payload;
		createInfo.sampler = sampler->sampler();
		return out_texture.construct(createInfo);
	}
	return false;
}

std::optional<AssetLoader<graphics::Texture>::Data> AssetLoader<graphics::Texture>::data(AssetLoadInfo<graphics::Texture> const& info) const {
	if (!info.m_data.bitmap.bytes.empty()) {
		if (!info.m_data.bitmap.compressed) {
			return info.m_data.bitmap;
		} else {
			return info.m_data.bitmap.bytes;
		}
	} else if (!info.m_data.cubemap.bytes[0].empty()) {
		if (!info.m_data.cubemap.compressed) {
			return info.m_data.cubemap;
		} else {
			return info.m_data.cubemap.bytes;
		}
	} else if (info.m_data.imageURIs.size() == 1) {
		auto path = info.m_data.prefix / info.m_data.imageURIs[0];
		path += info.m_data.ext;
		if (auto res = info.resource(path, Resource::Type::eBinary, Resources::Flag::eMonitor)) { return graphics::Texture::img(res->bytes()); }
	} else if (info.m_data.imageURIs.size() == 6) {
		graphics::Texture::Cube cube;
		std::size_t idx = 0;
		for (auto const& p : info.m_data.imageURIs) {
			auto path = info.m_data.prefix / p;
			path += info.m_data.ext;
			auto res = info.resource(path, Resource::Type::eBinary, Resources::Flag::eMonitor);
			if (!res) { return std::nullopt; }
			cube[idx++] = graphics::Texture::img(res->bytes());
		}
		return cube;
	}
	return std::nullopt;
}

std::unique_ptr<BitmapFont> AssetLoader<BitmapFont>::load(AssetLoadInfo<BitmapFont> const& info) const {
	BitmapFont font;
	if (load(font, info)) { return std::make_unique<BitmapFont>(std::move(font)); }
	return {};
}

bool AssetLoader<BitmapFont>::reload(BitmapFont& out_font, AssetLoadInfo<BitmapFont> const& info) const { return load(out_font, info); }

bool AssetLoader<BitmapFont>::load(BitmapFont& out_font, AssetLoadInfo<BitmapFont> const& info) const {
	auto const samplerURI = info.m_data.samplerURI == Hash{} ? "samplers/default" : info.m_data.samplerURI;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerURI);
	if (!sampler) { return false; }
	if (auto text = info.resource(info.m_data.jsonURI, Resource::Type::eText, Resources::Flag::eMonitor)) {
		dj::json json;
		auto result = json.read(text->string());
		if (result && result.errors.empty()) {
			auto const fi = io::fromJson<BitmapFontInfo>(json);
			auto const atlas = info.resource(info.m_data.jsonURI.parent_path() / fi.atlasURI, Resource::Type::eBinary, Resources::Flag::eMonitor);
			if (!atlas) { return false; }
			BitmapFont::CreateInfo bci;
			bci.forceFormat = info.m_data.forceFormat;
			bci.glyphs = fi.glyphs;
			bci.atlas = graphics::Texture::img(atlas->bytes());
			if (out_font.make(info.m_data.vram, *sampler, bci)) { return true; }
		}
	}
	return false;
}

std::unique_ptr<Model> AssetLoader<Model>::load(AssetLoadInfo<Model> const& info) const {
	static Hash const defaultSampler = "samplers/default";
	auto const samplerURI = info.m_data.samplerURI == Hash{} ? defaultSampler : info.m_data.samplerURI;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerURI);
	if (!sampler) { return {}; }
	if (auto mci = Model::load(info.m_data.modelURI, info.m_data.jsonURI, info.media())) {
		Model model;
		if (model.construct(info.m_data.vram, *mci, *sampler, info.m_data.forceFormat)) { return std::make_unique<Model>(std::move(model)); }
	}
	return {};
}

bool AssetLoader<Model>::reload(Model& out_model, AssetLoadInfo<Model> const& info) const {
	static Hash const defaultSampler = "samplers/default";
	auto const samplerURI = info.m_data.samplerURI == Hash{} ? defaultSampler : info.m_data.samplerURI;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerURI);
	if (!sampler) { return false; }
	if (auto mci = Model::load(info.m_data.modelURI, info.m_data.jsonURI, info.media())) {
		return out_model.construct(info.m_data.vram, std::move(mci).value(), *sampler, info.m_data.forceFormat).has_value();
	}
	return false;
}
} // namespace le
