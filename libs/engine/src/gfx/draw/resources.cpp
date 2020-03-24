#include <memory>
#include "resources.hpp"

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
}

#if defined(LEVK_RESOURCES_UPDATE)
void Resources::update()
{
	for (auto& [id, uResource] : m_resources.m_map)
	{
		uResource->update();
	}
	return;
}
#endif
} // namespace le::gfx
