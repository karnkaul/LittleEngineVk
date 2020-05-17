#include "engine/assets/resources.hpp"
#include "engine/gfx/font.hpp"
#include "engine/gfx/mesh.hpp"
#include "engine/gfx/shader.hpp"
#include "engine/gfx/texture.hpp"

namespace le
{
Resources& Resources::inst()
{
	static Resources s_inst;
	return s_inst;
}

Resources::Resources()
{
	m_bActive.store(false);
}

Resources::~Resources()
{
	ASSERT(!m_bActive && m_resources.count() == 0, "Resources singleton not deinitialised!");
	deinit();
}

void Resources::deinit()
{
	m_bActive.store(false);
	m_resources.unloadAll();
	return;
}

void Resources::update()
{
	if (!m_bActive.load())
	{
		return;
	}
	for (auto& [id, uResource] : m_resources.m_map)
	{
		uResource->update();
	}
	return;
}

void Resources::init(IOReader const& data)
{
	constexpr static std::array<u8, 4> white1pxBytes = {0xff, 0xff, 0xff, 0xff};
	constexpr static std::array<u8, 4> black1pxBytes = {0x0, 0x0, 0x0, 0x0};
	m_bActive.store(true);
	{
		create<gfx::Material>("materials/default", {});
	}
	{
		create<gfx::Sampler>("samplers/default", {});
		gfx::Sampler::Info info;
		info.mode = gfx::Sampler::Mode::eClampEdge;
		info.min = gfx::Sampler::Filter::eNearest;
		info.mip = gfx::Sampler::Filter::eNearest;
		create<gfx::Sampler>("samplers/font", std::move(info));
	}
	{
		gfx::Mesh::Info info;
		info.geometry = gfx::createCube();
		create<gfx::Mesh>("meshes/cube", std::move(info));
	}
	{
		gfx::Texture::Info info;
		info.raw.size = {1, 1};
		info.raw.bytes = ArrayView<u8>(white1pxBytes);
		create<gfx::Texture>("textures/white", info);
		info.raw.bytes = ArrayView<u8>(black1pxBytes);
		create<gfx::Texture>("textures/black", info);
	}
	{
		gfx::Cubemap::Info info;
		gfx::Texture::Raw b1px;
		b1px.bytes = ArrayView<u8>(black1pxBytes);
		b1px.size = {1, 1};
		info.rludfbRaw = {b1px, b1px, b1px, b1px, b1px, b1px};
		create<gfx::Cubemap>("cubemaps/blank", std::move(info));
	}
	{
		gfx::Shader::Info info;
		std::array shaderIDs = {stdfs::path("shaders/uber.vert"), stdfs::path("shaders/uber.frag")};
		ASSERT(data.checkPresences(shaderIDs), "Shaders missing!");
		info.pReader = &data;
		info.codeIDMap.at((size_t)gfx::Shader::Type::eVertex) = shaderIDs.at(0);
		info.codeIDMap.at((size_t)gfx::Shader::Type::eFragment) = shaderIDs.at(1);
		create<gfx::Shader>("shaders/default", std::move(info));
	}
	{
		gfx::Font::Info fontInfo;
		auto [str, bResult] = data.getString("fonts/default.json");
		ASSERT(bResult, "Default font not found!");
		fontInfo.deserialise(GData(std::move(str)));
		auto [img, bImg] = data.getBytes(stdfs::path("fonts") / fontInfo.sheetID);
		ASSERT(bImg, "Default font not found!");
		fontInfo.image = std::move(img);
		create<gfx::Font>("fonts/default", std::move(fontInfo));
	}
	return;
}
} // namespace le
