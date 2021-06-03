#include <algorithm>
#include <dumb_json/json.hpp>
#include <engine/assets/asset_loaders.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/config.hpp>
#include <graphics/utils/utils.hpp>

namespace le {
namespace {
bool isGlsl(io::Path const& path) { return path.has_extension() && (path.extension() == ".vert" || path.extension() == ".frag"); }
} // namespace

std::optional<graphics::Shader> AssetLoader<graphics::Shader>::load(AssetLoadInfo<graphics::Shader> const& info) const {
	auto const& paths = info.m_data.shaderPaths;
	if (!paths.empty() && std::all_of(paths.begin(), paths.end(), [&info](auto const& kvp) { return info.reader().present(kvp.second); })) {
		if (auto d = data(info)) { return graphics::Shader(info.m_data.device, info.m_data.name, std::move(*d)); }
	}
	return std::nullopt;
}

bool AssetLoader<graphics::Shader>::reload(graphics::Shader& out_shader, AssetLoadInfo<graphics::Shader> const& info) const {
	if (auto d = data(info)) { return out_shader.reconstruct(std::move(*d)); }
	return false;
}

std::optional<AssetLoader<graphics::Shader>::Data> AssetLoader<graphics::Shader>::data(AssetLoadInfo<graphics::Shader> const& info) const {
	graphics::Shader::SpirVMap spirV;
	io::FileReader const* pFR = nullptr;
	for (auto& [type, id] : info.m_data.shaderPaths) {
		auto path = id;
		if (isGlsl(path)) {
			if constexpr (!levk_shaderCompiler) {
				// Fallback to previously compiled shader
				path = graphics::utils::spirVpath(path);
			} else {
				if (!pFR && !(pFR = dynamic_cast<io::FileReader const*>(&info.reader()))) { return std::nullopt; }
				auto spv = graphics::utils::compileGlsl(pFR->fullPath(id));
				if (levk_debug) {
					// Compile release shader too
					graphics::utils::compileGlsl(pFR->fullPath(id), {}, {}, false);
				}
				if (!spv) {
					if constexpr (levk_debug) { ENSURE(false, "Failed to compile GLSL"); }
					// Fallback to previously compiled shader
					spv = graphics::utils::spirVpath(path);
				} else {
					// Ensure resource presence (and add monitor if supported)
					if (!info.resource(id, Resource::Type::eText, true)) { return std::nullopt; }
				}
				path = *spv;
			}
		}
		auto pRes = info.resource(path, Resource::Type::eBinary, false, true);
		if (!pRes) { return std::nullopt; }
		spirV[type] = {pRes->bytes().begin(), pRes->bytes().end()};
	}
	return spirV;
}

std::optional<graphics::Pipeline> AssetLoader<graphics::Pipeline>::load(AssetLoadInfo<graphics::Pipeline> const& info) const {
	if (auto shader = info.m_store->find<graphics::Shader>(info.m_data.shaderID)) {
		info.reloadDepend(*shader);
		auto pipeInfo = info.m_data.info ? *info.m_data.info : info.m_data.context->pipeInfo(info.m_data.flags);
		return info.m_data.context->makePipeline(info.m_data.name, shader->get(), pipeInfo);
	}
	return std::nullopt;
}

bool AssetLoader<graphics::Pipeline>::reload(graphics::Pipeline& out_pipe, AssetLoadInfo<graphics::Pipeline> const& info) const {
	if (auto shader = info.m_store->find<graphics::Shader>(info.m_data.shaderID)) { return out_pipe.reconstruct(shader->get()); }
	return false;
}

std::optional<graphics::Texture> AssetLoader<graphics::Texture>::load(AssetLoadInfo<graphics::Texture> const& info) const {
	auto const sampler = info.m_store->find<graphics::Sampler>(info.m_data.samplerID);
	if (!sampler) { return std::nullopt; }
	if (auto d = data(info)) {
		graphics::Texture::CreateInfo createInfo;
		createInfo.data = std::move(*d);
		createInfo.sampler = sampler->get().sampler();
		createInfo.forceFormat = info.m_data.forceFormat;
		createInfo.payload = info.m_data.payload;
		graphics::Texture ret(info.m_data.vram);
		if (ret.construct(createInfo)) { return ret; }
	}
	return std::nullopt;
}

bool AssetLoader<graphics::Texture>::reload(graphics::Texture& out_texture, AssetLoadInfo<graphics::Texture> const& info) const {
	auto const sampler = info.m_store->find<graphics::Sampler>(info.m_data.samplerID);
	if (!sampler) { return false; }
	if (auto d = data(info)) {
		graphics::Texture::CreateInfo createInfo;
		createInfo.data = std::move(*d);
		createInfo.forceFormat = info.m_data.forceFormat;
		createInfo.payload = info.m_data.payload;
		createInfo.sampler = sampler->get().sampler();
		return out_texture.construct(createInfo);
	}
	return false;
}

std::optional<AssetLoader<graphics::Texture>::Data> AssetLoader<graphics::Texture>::data(AssetLoadInfo<graphics::Texture> const& info) const {
	if (!info.m_data.bitmap.bytes.empty()) {
		if (info.m_data.rawBytes) {
			return info.m_data.bitmap;
		} else {
			return info.m_data.bitmap.bytes;
		}
	} else if (info.m_data.imageIDs.size() == 1) {
		auto path = info.m_data.prefix / info.m_data.imageIDs[0];
		path += info.m_data.ext;
		if (auto pRes = info.resource(path, Resource::Type::eBinary, true)) { return graphics::Texture::Img{pRes->bytes().begin(), pRes->bytes().end()}; }
	} else if (info.m_data.imageIDs.size() == 6) {
		graphics::Texture::Cubemap cubemap;
		std::size_t idx = 0;
		for (auto const& p : info.m_data.imageIDs) {
			auto path = info.m_data.prefix / p;
			path += info.m_data.ext;
			auto pRes = info.resource(path, Resource::Type::eBinary, true);
			if (!pRes) { return std::nullopt; }
			cubemap[idx++] = {pRes->bytes().begin(), pRes->bytes().end()};
		}
		return cubemap;
	}
	return std::nullopt;
}

namespace {
struct FontInfo {
	io::Path atlasID;
	kt::fixed_vector<graphics::Glyph, maths::max<u8>()> glyphs;
	s32 orgSizePt = 0;
};

graphics::Glyph deserialise(u8 c, dj::json_t const& json) {
	graphics::Glyph ret;
	ret.ch = c;
	ret.st = {json["x"].as<s32>(), json["y"].as<s32>()};
	ret.uv = ret.cell = {json["width"].as<s32>(), json["height"].as<s32>()};
	ret.offset = {json["originX"].as<s32>(), json["originY"].as<s32>()};
	auto const pAdvance = json.find("advance");
	ret.xAdv = pAdvance ? pAdvance->as<s32>() : ret.cell.x;
	if (auto pBlank = json.find("isBlank")) { ret.blank = pBlank->as<bool>(); }
	return ret;
}

FontInfo deserialise(dj::json_t const& json) {
	FontInfo ret;
	if (auto pAtlas = json.find("sheetID")) { ret.atlasID = pAtlas->as<std::string>(); }
	if (auto pSize = json.find("size")) { ret.orgSizePt = pSize->as<s32>(); }
	if (auto pGlyphsData = json.find("glyphs")) {
		for (auto& [key, value] : pGlyphsData->as<dj::map_t>()) {
			if (!key.empty()) {
				if (ret.glyphs.size() == ret.glyphs.capacity()) { break; }
				graphics::Glyph const glyph = deserialise((u8)key[0], *value);
				if (glyph.cell.x > 0 && glyph.cell.y > 0) {
					ret.glyphs.push_back(glyph);
				} else {
					conf::g_log.log(dl::level::warning, 1, "[{}] [BitmapFont] Could not deserialise Glyph '{}'!", conf::g_name, key[0]);
				}
			}
		}
	}
	return ret;
}
} // namespace

std::optional<BitmapFont> AssetLoader<BitmapFont>::load(AssetLoadInfo<BitmapFont> const& info) const {
	BitmapFont font;
	if (load(font, info)) { return font; }
	return std::nullopt;
}

bool AssetLoader<BitmapFont>::reload(BitmapFont& out_font, AssetLoadInfo<BitmapFont> const& info) const { return load(out_font, info); }

bool AssetLoader<BitmapFont>::load(BitmapFont& out_font, AssetLoadInfo<BitmapFont> const& info) const {
	auto const sampler = info.m_store->find<graphics::Sampler>(info.m_data.samplerID);
	if (!sampler) { return false; }
	if (auto text = info.resource(info.m_data.jsonID, Resource::Type::eText, true)) {
		dj::json_t json;
		auto result = json.read(text->string());
		if (result && result.errors.empty()) {
			FontInfo const fi = deserialise(json);
			auto const atlas = info.resource(info.m_data.jsonID.parent_path() / fi.atlasID, Resource::Type::eBinary, true);
			if (!atlas) { return false; }
			BitmapFont::CreateInfo bci;
			bci.forceFormat = info.m_data.forceFormat;
			bci.glyphs = fi.glyphs;
			bci.atlas = atlas->bytes();
			if (out_font.create(info.m_data.vram, sampler->get(), bci)) { return true; }
		}
	}
	return false;
}

std::optional<Model> AssetLoader<Model>::load(AssetLoadInfo<Model> const& info) const {
	auto const sampler = info.m_store->find<graphics::Sampler>(info.m_data.samplerID);
	if (!sampler) { return std::nullopt; }
	if (auto mci = Model::load(info.m_data.modelID, info.m_data.jsonID, info.reader())) {
		Model model;
		if (model.construct(info.m_data.vram, *mci, sampler->get(), info.m_data.forceFormat)) { return model; }
	}
	return std::nullopt;
}

bool AssetLoader<Model>::reload(Model& out_model, AssetLoadInfo<Model> const& info) const {
	auto const sampler = info.m_store->find<graphics::Sampler>(info.m_data.samplerID);
	if (!sampler) { return false; }
	if (auto mci = Model::load(info.m_data.modelID, info.m_data.jsonID, info.reader())) {
		return out_model.construct(info.m_data.vram, mci.move(), sampler->get(), info.m_data.forceFormat).has_result();
	}
	return false;
}
} // namespace le
