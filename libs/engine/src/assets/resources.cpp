#include <memory>
#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "engine/assets/resources.hpp"
#include "engine/gfx/mesh.hpp"
#include "engine/gfx/texture.hpp"

namespace le::gfx
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
	create<Material>("materials/default", {});
	create<Sampler>("samplers/default", {});
	Texture::Info textureInfo;
	static std::array<u8, 4> const white1pxBytes = {0xff, 0xff, 0xff, 0xff};
	textureInfo.raw.bytes = ArrayView<u8>(white1pxBytes);
	textureInfo.raw.size = {1, 1};
	create<Texture>("textures/blank", textureInfo);
	return;
}
} // namespace le::gfx
