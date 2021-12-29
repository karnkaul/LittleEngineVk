#include <core/log_channel.hpp>
#include <core/services.hpp>
#include <dumb_json/json.hpp>
#include <engine/assets/asset_converters.hpp>
#include <engine/assets/asset_loaders.hpp>
#include <engine/assets/asset_store.hpp>
#include <graphics/render/pipeline_factory.hpp>
#include <graphics/utils/utils.hpp>
#include <algorithm>

namespace le {
using Payload = graphics::Texture::Payload;

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

std::unique_ptr<graphics::Texture> AssetLoader<graphics::Texture>::load(AssetLoadInfo<graphics::Texture> const& info) const {
	auto const samplerURI = info.m_data.samplerURI == Hash{} ? "samplers/default" : info.m_data.samplerURI;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerURI);
	if (!sampler) { return {}; }
	if (auto d = data(info)) {
		graphics::Texture out_texture(info.m_data.vram, sampler->sampler());
		if (load(out_texture, *d, sampler->sampler(), info.m_data.forceFormat)) { return std::make_unique<graphics::Texture>(std::move(out_texture)); }
	}
	return {};
}

bool AssetLoader<graphics::Texture>::reload(graphics::Texture& out_texture, AssetLoadInfo<graphics::Texture> const& info) const {
	auto const samplerURI = info.m_data.samplerURI == Hash{} ? "samplers/default" : info.m_data.samplerURI;
	if (auto d = data(info)) {
		auto const sampler = info.m_store->find<graphics::Sampler>(samplerURI);
		vk::Sampler const s = sampler ? sampler->sampler() : vk::Sampler();
		return load(out_texture, *d, s, info.m_data.forceFormat);
	}
	return false;
}

std::optional<AssetLoader<graphics::Texture>::Data> AssetLoader<graphics::Texture>::data(AssetLoadInfo<graphics::Texture> const& info) const {
	if (!info.m_data.bitmap.bytes.empty()) {
		return info.m_data.bitmap;
	} else if (!info.m_data.cubemap.bytes[0].empty()) {
		return info.m_data.cubemap;
	} else if (info.m_data.imageURIs.size() == 1) {
		auto path = info.m_data.prefix / info.m_data.imageURIs[0];
		path += info.m_data.ext;
		if (auto res = info.resource(path, Resource::Type::eBinary, Resources::Flag::eMonitor)) { return res->bytes(); }
	} else if (info.m_data.imageURIs.size() == 6) {
		Cube cube;
		std::size_t idx = 0;
		for (auto const& p : info.m_data.imageURIs) {
			auto path = info.m_data.prefix / p;
			path += info.m_data.ext;
			auto res = info.resource(path, Resource::Type::eBinary, Resources::Flag::eMonitor);
			if (!res) { return std::nullopt; }
			cube[idx++] = res->bytes();
		}
		return cube;
	}
	return std::nullopt;
}

bool AssetLoader<graphics::Texture>::load(graphics::Texture& out_texture, Data const& data, vk::Sampler sampler, std::optional<vk::Format> format) const {
	auto construct = [&out_texture, sampler, format](auto const& data) {
		if (format) {
			if (out_texture.construct(data, Payload::eColour, *format)) {
				if (sampler) { out_texture.changeSampler(sampler); }
				return true;
			}
		} else {
			if (out_texture.construct(data, Payload::eColour)) {
				if (sampler) { out_texture.changeSampler(sampler); }
				return true;
			}
		}
		return false;
	};
	if (auto bitmap = std::get_if<graphics::Bitmap>(&data)) {
		return construct(*bitmap);
	} else if (auto bytes = std::get_if<graphics::ImageData>(&data)) {
		return construct(*bytes);
	} else if (auto cubemap = std::get_if<graphics::Cubemap>(&data)) {
		return construct(*cubemap);
	} else if (auto bytes = std::get_if<Cube>(&data)) {
		return construct(Span<graphics::ImageData const>(*bytes));
	}
	return false;
}

std::unique_ptr<graphics::Font> AssetLoader<graphics::Font>::load(AssetLoadInfo<graphics::Font> const& info) const {
	if (auto ttf = info.resource(info.m_data.ttfURI, Resource::Type::eBinary, {})) {
		auto fi = info.m_data.info;
		fi.ttf = ttf->bytes();
		if (fi.name.empty()) { fi.name = info.m_data.ttfURI.filename().string(); }
		return std::make_unique<graphics::Font>(info.m_data.vram, fi);
	}
	return {};
}

bool AssetLoader<graphics::Font>::reload(graphics::Font& out_font, AssetLoadInfo<graphics::Font> const& info) const {
	if (auto ttf = info.resource(info.m_data.ttfURI, Resource::Type::eBinary, Resources::Flag::eReload)) {
		auto fi = info.m_data.info;
		fi.ttf = ttf->bytes();
		if (fi.name.empty()) { fi.name = info.m_data.ttfURI.filename().string(); }
		out_font = graphics::Font(out_font.m_vram, std::move(fi));
		return true;
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
