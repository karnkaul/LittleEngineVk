#include <memory>
#include "core/log.hpp"
#include "core/map_store.hpp"
#include "resources.hpp"

namespace le::gfx
{
namespace
{
struct Store
{
	TMapStore<std::unordered_map<std::string, std::unique_ptr<Shader>>> shaders;
};

Store g_store;
} // namespace

TResult<Shader*> resources::create(stdfs::path const& id, ShaderData const& data)
{
	auto uShader = std::make_unique<Shader>(data);
	auto const idStr = id.generic_string();
	g_store.shaders.insert(idStr, std::move(uShader));
	LOG_I("[{}] [{}] created", Shader::s_tName, idStr);
	return get<Shader>(idStr);
}

template <>
TResult<Shader*> resources::get<Shader>(stdfs::path const& id)
{
	auto [uShader, bResult] = g_store.shaders.get(id.generic_string());
	if (bResult)
	{
		return uShader->get();
	}
	return nullptr;
}

template <>
bool resources::unload<Shader>(stdfs::path const& id)
{
	auto const idStr = id.generic_string();
	if (g_store.shaders.unload(idStr))
	{
		LOG_I("[{}] [{}] destroyed", Shader::s_tName, idStr);
		return true;
	}
	return false;
}

template <>
void resources::unloadAll<Shader>()
{
	for (auto& [id, shader] : g_store.shaders.m_map)
	{
		LOG_I("[{}] [{}] destroyed", Shader::s_tName, id);
	}
	g_store.shaders.unloadAll();
	return;
}

void resources::update()
{
#if defined(LEVK_SHADER_HOT_RELOAD)
	for (auto& [id, uShader] : g_store.shaders.m_map)
	{
		uShader->update();
	}
#endif
	return;
}

void resources::unloadAll()
{
	unloadAll<Shader>();
	return;
}
} // namespace le::gfx
