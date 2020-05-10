#include <memory>
#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "engine/assets/resources.hpp"
#include "engine/gfx/font.hpp"
#include "engine/gfx/mesh.hpp"
#include "engine/gfx/texture.hpp"

namespace le
{
namespace
{
std::unique_ptr<Resources> g_uResources;
} // namespace

Resources::Service::Service()
{
	if (!g_uResources)
	{
		g_uResources = std::make_unique<Resources>();
		g_pResources = g_uResources.get();
		g_pResources->init();
	}
}

Resources::Service::~Service()
{
	g_uResources.reset();
	g_pResources = nullptr;
}

Resources::Resources() = default;

Resources::~Resources()
{
	unloadAll();
}

void Resources::unloadAll()
{
	m_resources.unloadAll();
	return;
}

void Resources::update()
{
	for (auto& [id, uResource] : m_resources.m_map)
	{
		uResource->update();
	}
	return;
}

void Resources::init()
{
	auto pDefault = create<gfx::Material>("materials/default", {});
	create<gfx::Material>("materials/font", {});
	create<gfx::Sampler>("samplers/default", {});
	gfx::Mesh::Info cubeInfo;
	cubeInfo.material.pMaterial = pDefault;
	cubeInfo.geometry = gfx::createCube();
	create<gfx::Mesh>("meshes/cube", std::move(cubeInfo));
	gfx::Sampler::Info fontSampler;
	fontSampler.mode = gfx::Sampler::Mode::eClampEdge;
	fontSampler.min = gfx::Sampler::Filter::eNearest;
	fontSampler.mip = gfx::Sampler::Filter::eNearest;
	create<gfx::Sampler>("samplers/font", std::move(fontSampler));
	gfx::Texture::Info textureInfo;
	static std::array<u8, 4> const white1pxBytes = {0xff, 0xff, 0xff, 0xff};
	static std::array<u8, 4> const black1pxBytes = {0x0, 0x0, 0x0, 0x0};
	textureInfo.raw.size = {1, 1};
	textureInfo.raw.bytes = ArrayView<u8>(white1pxBytes);
	create<gfx::Texture>("textures/white", textureInfo);
	textureInfo.raw.bytes = ArrayView<u8>(black1pxBytes);
	create<gfx::Texture>("textures/black", textureInfo);
	gfx::Cubemap::Info cubemapInfo;
	gfx::Texture::Raw b1px;
	b1px.bytes = ArrayView<u8>(black1pxBytes);
	b1px.size = {1, 1};
	cubemapInfo.rludfbRaw = {b1px, b1px, b1px, b1px, b1px, b1px};
	create<gfx::Cubemap>("cubemaps/blank", std::move(cubemapInfo));
	return;
}
} // namespace le
