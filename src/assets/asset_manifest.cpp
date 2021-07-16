#include <core/utils/enum_values.hpp>
#include <engine/assets/asset_loaders.hpp>
#include <engine/assets/asset_manifest.hpp>
#include <engine/engine.hpp>
#include <graphics/context/bootstrap.hpp>

namespace le {
void AssetManifest::append(AssetManifest const& rhs) {
	m_samplers = m_samplers + rhs.m_samplers;
	m_textures = m_textures + rhs.m_textures;
}

std::size_t AssetManifest::preload(dj::json_t const& root) {
	std::size_t ret{};
	for (auto const& [groupName, entries] : root.as<dj::map_t>()) {
		Group group;
		for (auto const& entry : entries->as<dj::vec_t>()) {
			if (auto id = entry->find("id"); id && id->is_string()) { group.emplace(id->as<std::string>(), entry); }
		}
		if (!group.empty()) { ret += add(groupName, std::move(group)); }
	}
	return ret;
}

void AssetManifest::load(dts::scheduler* scheduler) {
	m_deps[Kind::eSampler] = m_loader.stage(std::move(m_samplers), scheduler);
	m_deps[Kind::eTexture] = m_loader.stage(std::move(m_textures), scheduler, m_deps[Kind::eSampler]);
	m_deps[Kind::eShader] = m_loader.stage(std::move(m_shaders), scheduler);
	m_deps[Kind::ePipeline] = m_loader.stage(std::move(m_pipelines), scheduler, m_deps[Kind::ePipeline]);
	loadCustom(scheduler);
}

std::vector<AssetManifest::StageID> AssetManifest::deps(Flags flags) const noexcept {
	std::vector<StageID> ret;
	ret.reserve(std::size_t(Kind::eCOUNT_));
	for (Kind const k : utils::EnumValues<Kind>()) {
		if (flags.test(k)) { ret.push_back(m_deps[k]); }
	}
	return ret;
}

not_null<graphics::Device*> AssetManifest::device() { return m_device ? m_device : (m_device = &Services::locate<Engine>()->gfx().boot.device); }
not_null<graphics::VRAM*> AssetManifest::vram() { return m_vram ? m_vram : (m_vram = &Services::locate<Engine>()->gfx().boot.vram); }

std::size_t AssetManifest::add(std::string_view groupName, Group group) {
	if (groupName == "samplers") { return addSamplers(std::move(group)); }
	if (groupName == "shaders") { return addShaders(std::move(group)); }
	if (groupName == "textures") { return addTextures(std::move(group)); }
	/*if (groupName == "pipeline") { return Kind::ePipeline; }
	if (groupName == "bitmap_fonts") { return Kind::eBitmapFont; }
	if (groupName == "models") { return Kind::eModel; }
	if (groupName == "draw_layers") { return Kind::eDrawLayer; }*/
	return addCustom(std::move(group));
}

std::size_t AssetManifest::addSamplers(Group group) {
	std::size_t ret{};
	auto dv = device();
	for (auto& [id, entry] : group) {
		if (entry->contains("min") && entry->contains("mag")) {
			graphics::TPair<vk::Filter> minMag;
			minMag.first = (*entry)["min"].as<std::string>() == "nearest" ? vk::Filter::eNearest : vk::Filter::eLinear;
			minMag.second = (*entry)["mag"].as<std::string>() == "nearest" ? vk::Filter::eNearest : vk::Filter::eLinear;
			vk::SamplerCreateInfo createInfo;
			if (auto mipMode = entry->find("mip_mode")) {
				auto const mm = mipMode->as<std::string>() == "nearest" ? vk::SamplerMipmapMode::eNearest : vk::SamplerMipmapMode::eLinear;
				createInfo = graphics::Sampler::info(minMag, mm);
			} else {
				createInfo = graphics::Sampler::info(minMag);
			}
			m_samplers.add(std::move(id), [createInfo, dv]() { return graphics::Sampler(dv, createInfo); });
			++ret;
		}
	}
	return ret;
}

std::size_t AssetManifest::addShaders(Group group) {
	std::size_t ret{};
	for (auto& [id, entry] : group) {
		if (auto files = entry->find("files")) {
			AssetLoadData<graphics::Shader> data(device());
			data.name = id.generic_string();
			for (auto const& str : files->as<std::vector<std::string>>()) {
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
	auto vram = Services::locate<graphics::VRAM>();
	for (auto& [id, entry] : group) {
		using Payload = graphics::Texture::Payload;
		AssetLoadData<graphics::Texture> data(vram);
		if (false) {

		} else if (auto files = entry->find("files"); files && files->is_array()) {
			auto const f = files->as<std::vector<std::string>>();
			data.imageIDs = {f.begin(), f.end()};
		} else if (auto file = entry->find("file"); file && file->is_string()) {
			data.imageIDs = {file->as<std::string>()};
		} else {
			data.imageIDs = {id.generic_string()};
		}
		if (auto prefix = entry->find("prefix"); prefix && prefix->is_string()) { data.prefix = prefix->as<std::string>(); }
		if (auto ext = entry->find("ext"); ext && ext->is_string()) { data.ext = ext->as<std::string>(); }
		if (auto sampler = entry->find("sampler"); sampler && sampler->is_string()) { data.samplerID = sampler->as<std::string>(); }
		if (auto payload = entry->find("payload"); payload && payload->is_string()) {
			data.payload = payload->as<std::string>() == "data" ? Payload::eData : Payload::eColour;
		}
		m_textures.add(id, std::move(data));
		++ret;
	}
	return ret;
}
} // namespace le
