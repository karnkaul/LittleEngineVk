#include <algorithm>
#include <dumb_json/json.hpp>
#include <engine/assets/asset_loaders.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/utils/logger.hpp>
#include <graphics/utils/utils.hpp>

namespace le {
namespace {
bool isGlsl(io::Path const& path) { return path.has_extension() && (path.extension() == ".vert" || path.extension() == ".frag"); }

template <bool D = levk_debug>
io::Path spirvPath(io::Path const& glsl, io::FileReader const& reader);

template <>
[[maybe_unused]] io::Path spirvPath<true>(io::Path const& glsl, io::FileReader const& reader) {
	auto spv = graphics::utils::spirVpath(glsl);
	if (auto res = graphics::utils::compileGlsl(reader.fullPath(glsl))) {
		spv = *res;
	} else {
		ensure(false, "Failed to compile GLSL");
	}
	// compile Release shader too
	graphics::utils::compileGlsl(reader.fullPath(glsl), {}, {}, false);
	return spv;
}

template <>
[[maybe_unused]] io::Path spirvPath<false>(io::Path const& glsl, io::FileReader const& reader) {
	auto spv = graphics::utils::spirVpath(glsl);
	if (!reader.checkPresence(spv)) {
		if (auto res = graphics::utils::compileGlsl(reader.fullPath(glsl))) { spv = *res; }
	}
	return spv;
}
} // namespace

std::unique_ptr<graphics::Shader> AssetLoader<graphics::Shader>::load(AssetLoadInfo<graphics::Shader> const& info) const {
	auto const& paths = info.m_data.shaderPaths;
	if (!paths.empty() && std::all_of(paths.begin(), paths.end(), [&info](auto const& kvp) { return info.reader().present(kvp.second); })) {
		if (auto d = data(info)) { return std::make_unique<graphics::Shader>(info.m_data.device, info.m_data.name, std::move(*d)); }
	}
	return {};
}

bool AssetLoader<graphics::Shader>::reload(graphics::Shader& out_shader, AssetLoadInfo<graphics::Shader> const& info) const {
	if (auto d = data(info)) { return out_shader.reconstruct(std::move(*d)); }
	return false;
}

std::optional<AssetLoader<graphics::Shader>::Data> AssetLoader<graphics::Shader>::data(AssetLoadInfo<graphics::Shader> const& info) const {
	graphics::Shader::SpirVMap spirV;
	io::FileReader const* fr = nullptr;
	for (auto& [type, id] : info.m_data.shaderPaths) {
		auto path = id;
		if (isGlsl(path)) {
			if (!fr && !(fr = dynamic_cast<io::FileReader const*>(&info.reader()))) {
				// cannot compile shaders without FileReader
				path = graphics::utils::spirVpath(id);
			} else {
				// ensure resource presence (and add monitor if supported)
				if (!info.resource(id, Resource::Type::eText, true)) { return std::nullopt; }
				path = spirvPath(id, *fr);
			}
		} else {
			// fallback to previously compiled shader
			path = graphics::utils::spirVpath(id);
		}
		auto res = info.resource(path, Resource::Type::eBinary, false, true);
		if (!res) { return std::nullopt; }
		spirV[type] = {res->bytes().begin(), res->bytes().end()};
	}
	return spirV;
}

namespace {
void setup(graphics::Pipeline::CreateInfo::Fixed& out, AssetLoadData<graphics::Pipeline>::Variant const& variant) {
	out.topology = variant.topology;
	out.rasterizerState.polygonMode = variant.polygonMode;
	if (variant.lineWidth > 0.0f) { out.rasterizerState.lineWidth = variant.lineWidth; }
}
} // namespace

std::unique_ptr<graphics::Pipeline> AssetLoader<graphics::Pipeline>::load(AssetLoadInfo<graphics::Pipeline> const& info) const {
	if (auto shader = info.m_store->find<graphics::Shader>(info.m_data.shaderID)) {
		info.reloadDepend(shader);
		auto pipeInfo = info.m_data.info ? *info.m_data.info : info.m_data.context->pipeInfo(info.m_data.flags);
		pipeInfo.renderPass = info.m_data.gui ? info.m_data.context->renderer().renderPassUI() : info.m_data.context->renderer().renderPass3D();
		setup(pipeInfo.fixedState, info.m_data.main);
		auto ret = info.m_data.context->makePipeline(info.m_data.name, *shader, pipeInfo);
		for (auto const& variant : info.m_data.variants) {
			if (variant.id > Hash()) {
				auto fixed = ret.fixedState();
				setup(fixed, variant);
				auto const res = ret.constructVariant(variant.id, fixed);
				ensure(res.has_value(), "Pipeline variant construction failure");
			}
		}
		return std::make_unique<graphics::Pipeline>(std::move(ret));
	}
	return {};
}

bool AssetLoader<graphics::Pipeline>::reload(graphics::Pipeline& out_pipe, AssetLoadInfo<graphics::Pipeline> const& info) const {
	if (auto shader = info.m_store->find<graphics::Shader>(info.m_data.shaderID)) { return out_pipe.reconstruct(*shader); }
	return false;
}

std::unique_ptr<graphics::Texture> AssetLoader<graphics::Texture>::load(AssetLoadInfo<graphics::Texture> const& info) const {
	auto const samplerID = info.m_data.samplerID == Hash{} ? "samplers/default" : info.m_data.samplerID;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerID);
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
	auto const samplerID = info.m_data.samplerID == Hash{} ? "samplers/default" : info.m_data.samplerID;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerID);
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
	} else if (info.m_data.imageIDs.size() == 1) {
		auto path = info.m_data.prefix / info.m_data.imageIDs[0];
		path += info.m_data.ext;
		if (auto res = info.resource(path, Resource::Type::eBinary, true)) { return graphics::Texture::img(res->bytes()); }
	} else if (info.m_data.imageIDs.size() == 6) {
		graphics::Texture::Cube cube;
		std::size_t idx = 0;
		for (auto const& p : info.m_data.imageIDs) {
			auto path = info.m_data.prefix / p;
			path += info.m_data.ext;
			auto res = info.resource(path, Resource::Type::eBinary, true);
			if (!res) { return std::nullopt; }
			cube[idx++] = graphics::Texture::img(res->bytes());
		}
		return cube;
	}
	return std::nullopt;
}

namespace {
struct FontInfo {
	io::Path atlasID;
	ktl::fixed_vector<graphics::BitmapGlyph, maths::max<u8>()> glyphs;
	s32 orgSizePt = 0;
};

graphics::BitmapGlyph deserialise(u8 c, dj::json const& json) {
	graphics::BitmapGlyph ret;
	ret.ch = c;
	ret.st = {json["x"].as<s32>(), json["y"].as<s32>()};
	ret.uv = ret.cell = {json["width"].as<s32>(), json["height"].as<s32>()};
	ret.offset = {json["originX"].as<s32>(), json["originY"].as<s32>()};
	auto const pAdvance = json.find("advance");
	ret.xAdv = pAdvance ? pAdvance->as<s32>() : ret.cell.x;
	if (auto pBlank = json.find("isBlank")) { ret.blank = pBlank->as<bool>(); }
	return ret;
}

FontInfo deserialise(dj::json const& json) {
	FontInfo ret;
	if (auto pAtlas = json.find("sheetID")) { ret.atlasID = pAtlas->as<std::string>(); }
	if (auto pSize = json.find("size")) { ret.orgSizePt = pSize->as<s32>(); }
	if (auto pGlyphsData = json.find("glyphs")) {
		for (auto& [key, value] : pGlyphsData->as<dj::map_t>()) {
			if (!key.empty()) {
				if (ret.glyphs.size() == ret.glyphs.capacity()) { break; }
				graphics::BitmapGlyph const glyph = deserialise((u8)key[0], *value);
				if (glyph.cell.x > 0 && glyph.cell.y > 0) {
					ret.glyphs.push_back(glyph);
				} else {
					utils::g_log.log(dl::level::warning, 1, "[{}] [BitmapFont] Could not deserialise Glyph '{}'!", utils::g_name, key[0]);
				}
			}
		}
	}
	return ret;
}
} // namespace

std::unique_ptr<BitmapFont> AssetLoader<BitmapFont>::load(AssetLoadInfo<BitmapFont> const& info) const {
	BitmapFont font;
	if (load(font, info)) { return std::make_unique<BitmapFont>(std::move(font)); }
	return {};
}

bool AssetLoader<BitmapFont>::reload(BitmapFont& out_font, AssetLoadInfo<BitmapFont> const& info) const { return load(out_font, info); }

bool AssetLoader<BitmapFont>::load(BitmapFont& out_font, AssetLoadInfo<BitmapFont> const& info) const {
	auto const samplerID = info.m_data.samplerID == Hash{} ? "samplers/default" : info.m_data.samplerID;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerID);
	if (!sampler) { return false; }
	if (auto text = info.resource(info.m_data.jsonID, Resource::Type::eText, true)) {
		dj::json json;
		auto result = json.read(text->string());
		if (result && result.errors.empty()) {
			FontInfo const fi = deserialise(json);
			auto const atlas = info.resource(info.m_data.jsonID.parent_path() / fi.atlasID, Resource::Type::eBinary, true);
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
	auto const samplerID = info.m_data.samplerID == Hash{} ? "samplers/default" : info.m_data.samplerID;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerID);
	if (!sampler) { return {}; }
	if (auto mci = Model::load(info.m_data.modelID, info.m_data.jsonID, info.reader())) {
		Model model;
		if (model.construct(info.m_data.vram, *mci, *sampler, info.m_data.forceFormat)) { return std::make_unique<Model>(std::move(model)); }
	}
	return {};
}

bool AssetLoader<Model>::reload(Model& out_model, AssetLoadInfo<Model> const& info) const {
	auto const samplerID = info.m_data.samplerID == Hash{} ? "samplers/default" : info.m_data.samplerID;
	auto const sampler = info.m_store->find<graphics::Sampler>(samplerID);
	if (!sampler) { return false; }
	if (auto mci = Model::load(info.m_data.modelID, info.m_data.jsonID, info.reader())) {
		return out_model.construct(info.m_data.vram, std::move(mci).value(), *sampler, info.m_data.forceFormat).has_value();
	}
	return false;
}
} // namespace le
