#include <algorithm>
#include <engine/assets/asset_loaders.hpp>
#include <engine/assets/asset_store.hpp>
#include <graphics/utils/utils.hpp>

namespace le {
namespace {
bool isGlsl(io::Path const& path) {
	return path.has_extension() && (path.extension() == ".vert" || path.extension() == ".frag");
}
} // namespace

std::optional<graphics::Shader> AssetLoader<graphics::Shader>::load(AssetLoadInfo<graphics::Shader> const& info) const {
	auto const& paths = info.m_data.shaderPaths;
	if (!paths.empty() && std::all_of(paths.begin(), paths.end(), [&info](auto const& kvp) { return info.reader().isPresent(kvp.second); })) {
		if (auto d = data(info)) {
			return graphics::Shader(info.m_data.device, std::move(*d));
		}
	}
	return std::nullopt;
}

bool AssetLoader<graphics::Shader>::reload(graphics::Shader& out_shader, AssetLoadInfo<graphics::Shader> const& info) const {
	if (auto d = data(info)) {
		return out_shader.reconstruct(std::move(*d));
	}
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
				if (!pFR && !(pFR = dynamic_cast<io::FileReader const*>(&info.reader()))) {
					return std::nullopt;
				}
				auto spv = graphics::utils::compileGlsl(pFR->fullPath(id));
				if (levk_debug) {
					// Compile release shader too
					graphics::utils::compileGlsl(pFR->fullPath(id), {}, {}, false);
				}
				if (!spv) {
					// Fallback to previously compiled shader
					spv = graphics::utils::spirVpath(path);
				} else {
					// Ensure resource presence (and add monitor if supported)
					if (!info.resource(id, Resource::Type::eText, true)) {
						return std::nullopt;
					}
				}
				path = *spv;
			}
		}
		auto pRes = info.resource(path, Resource::Type::eBinary, false, true);
		if (!pRes) {
			return std::nullopt;
		}
		spirV[(std::size_t)type] = {pRes->bytes().begin(), pRes->bytes().end()};
	}
	return spirV;
}

std::optional<graphics::Pipeline> AssetLoader<graphics::Pipeline>::load(AssetLoadInfo<graphics::Pipeline> const& info) const {
	if (auto shader = info.m_store.get().find<graphics::Shader>(info.m_data.shaderID)) {
		info.reloadDepend(*shader);
		auto pipeInfo = info.m_data.info ? *info.m_data.info : info.m_data.context.get().pipeInfo(info.m_data.flags);
		return info.m_data.context.get().makePipeline(info.m_data.name, shader->get(), pipeInfo);
	}
	return std::nullopt;
}

bool AssetLoader<graphics::Pipeline>::reload(graphics::Pipeline& out_pipe, AssetLoadInfo<graphics::Pipeline> const& info) const {
	if (auto shader = info.m_store.get().find<graphics::Shader>(info.m_data.shaderID)) {
		return out_pipe.reconstruct(shader->get());
	}
	return false;
}

std::optional<graphics::Texture> AssetLoader<graphics::Texture>::load(AssetLoadInfo<graphics::Texture> const& info) const {
	auto const sampler = info.m_store.get().find<graphics::Sampler>(info.m_data.samplerID);
	if (!sampler) {
		return std::nullopt;
	}
	if (auto d = data(info)) {
		graphics::Texture::CreateInfo createInfo;
		createInfo.data = std::move(*d);
		createInfo.sampler = sampler->get().sampler();
		graphics::Texture ret(info.m_data.name, info.m_data.vram);
		if (ret.construct(createInfo)) {
			return ret;
		}
	}
	return std::nullopt;
}

bool AssetLoader<graphics::Texture>::reload(graphics::Texture& out_texture, AssetLoadInfo<graphics::Texture> const& info) const {
	auto const sampler = info.m_store.get().find<graphics::Sampler>(info.m_data.samplerID);
	if (!sampler) {
		return false;
	}
	if (auto d = data(info)) {
		graphics::Texture::CreateInfo createInfo;
		createInfo.data = std::move(*d);
		createInfo.sampler = sampler->get().sampler();
		return out_texture.construct(createInfo);
	}
	return false;
}

std::optional<AssetLoader<graphics::Texture>::Data> AssetLoader<graphics::Texture>::data(AssetLoadInfo<graphics::Texture> const& info) const {
	if (!info.m_data.raw.bytes.empty()) {
		return info.m_data.raw;
	} else if (info.m_data.imageIDs.size() == 1) {
		auto path = info.m_data.prefix / info.m_data.imageIDs[0];
		path += info.m_data.ext;
		if (auto pRes = info.resource(path, Resource::Type::eBinary, true)) {
			return graphics::Texture::Img{{pRes->bytes().begin(), pRes->bytes().end()}};
		}
	} else if (info.m_data.imageIDs.size() == 6) {
		graphics::Texture::Cubemap cubemap;
		std::size_t idx = 0;
		for (auto const& p : info.m_data.imageIDs) {
			auto path = info.m_data.prefix / p;
			path += info.m_data.ext;
			auto pRes = info.resource(path, Resource::Type::eBinary, true);
			if (!pRes) {
				return std::nullopt;
			}
			cubemap.bytes[idx++] = {pRes->bytes().begin(), pRes->bytes().end()};
		}
		return cubemap;
	}
	return std::nullopt;
}
} // namespace le
