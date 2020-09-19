#include <core/assert.hpp>
#include <core/log.hpp>
#include <engine/game/driver.hpp>
#include <engine/game/state.hpp>
#include <resources/manifest.hpp>
#include <game/state_impl.hpp>
#include <editor/editor.hpp>

namespace le
{
namespace
{
struct
{
	std::optional<engine::Semaphore> engineSemaphore;
	res::Manifest manifest;
	gs::ManifestLoaded onManifestLoaded;
#if defined(LEVK_EDITOR)
	Transform root;
	gs::EMap entityMap;
#endif
} g_ctxImpl;
} // namespace

bool Prop::valid() const noexcept
{
	return pTransform != nullptr && entity.id != ECSID::null;
}

Transform const& Prop::transform() const
{
	ASSERT(pTransform, "Null Transform!");
	return *pTransform;
}

Transform& Prop::transform()
{
	ASSERT(pTransform, "Null Transform!");
	return *pTransform;
}

void gs::Context::reset()
{
	Registry& reg = registry;
	reg.clear();
	name.clear();
	gameRect = {};
	camera = {};
#if defined(LEVK_EDITOR)
	editorData = {};
#endif
}

Token gs::registerInput(input::Context const* pContext)
{
	ASSERT(pContext, "Context is null!");
	return input::registerContext(pContext);
}

TResult<Token> gs::loadManifest(LoadReq const& loadReq)
{
	if (!g_ctxImpl.manifest.idle())
	{
		LOG_W("[le::GameState] Manifest load already in progress!");
		return {};
	}
	res::ResourceList unload;
	TResult<Token> ret;
	if (!loadReq.unload.empty() && engine::reader().checkPresence(loadReq.unload))
	{
		res::Manifest temp;
		temp.read(loadReq.unload);
		unload = temp.parse();
	}
	if (!loadReq.load.empty() && engine::reader().checkPresence(loadReq.load))
	{
		g_ctxImpl.manifest.read(loadReq.load);
		auto const loadList = g_ctxImpl.manifest.parse();
		if (!loadList.empty())
		{
			LOG_I("[le::GameState] Loading manifest [{}] [{} resources]", loadReq.load.generic_string(), loadList.size());
			unload = unload - loadList;
			ret = g_ctxImpl.onManifestLoaded.subscribe(loadReq.onLoaded);
			g_ctxImpl.manifest.start();
			g_ctxImpl.engineSemaphore = engine::setBusy();
		}
		else
		{
			g_ctxImpl.manifest.reset();
		}
	}
	if (!unload.empty())
	{
		LOG_I("[le::GameState] Unloading manifest [{}] [{} resources]", loadReq.unload.generic_string(), unload.size());
		res::Manifest::unload(unload);
	}
	return ret;
}

Token gs::loadInputMap(stdfs::path const& id, input::Context* out_pContext)
{
	if (!id.empty() && engine::reader().isPresent(id))
	{
		if (auto str = engine::reader().string(id))
		{
			dj::object json;
			if (json.read(*str))
			{
				if (auto const parsed = out_pContext->deserialise(json); parsed > 0)
				{
					LOG_D("[le::GameState] Parsed [{}] input mappings from [{}]", parsed, id.generic_string());
				}
			}
			else
			{
				LOG_W("[le::GameState] Failed to read input mappings from [{}]!", id.generic_string());
			}
		}
	}
	return registerInput(out_pContext);
}

void gs::destroy(Span<Prop> props)
{
	Registry& reg = g_context.registry;
	for (auto& prop : props)
	{
		if (reg.destroy(prop.entity))
		{
#if defined(LEVK_EDITOR)
			g_ctxImpl.entityMap.erase(prop.pTransform);
#endif
		}
	}
}

void gs::reset()
{
	Registry& reg = g_context.registry;
	reg.clear();
	g_context.camera = {};
	g_context.gameRect = {};
	g_ctxImpl.manifest.reset();
	g_ctxImpl.onManifestLoaded.clear();
#if defined(LEVK_EDITOR)
	g_ctxImpl.entityMap.clear();
	g_ctxImpl.root.reset(true);
#endif
}

gfx::Renderer::Scene gs::update(engine::Driver& out_driver, Time dt, bool bTick)
{
	if (!g_ctxImpl.manifest.idle())
	{
		auto const status = g_ctxImpl.manifest.update(!bTick);
		switch (status)
		{
		case res::Manifest::Status::eIdle:
		{
			g_ctxImpl.engineSemaphore = {};
			g_ctxImpl.manifest.reset();
			if (bTick)
			{
				g_ctxImpl.onManifestLoaded();
			}
			g_ctxImpl.onManifestLoaded.clear();
			break;
		}
		default:
		{
			break;
		}
		}
	}
	if (bTick)
	{
		out_driver.tick(dt);
	}
	Ref<gfx::Camera> camera = g_context.camera;
#if defined(LEVK_EDITOR)
	if (!editor::g_bTickGame)
	{
		camera = editor::g_editorCam.m_camera;
	}
#endif
	return out_driver.builder().build(camera, g_context.registry);
}

#if defined(LEVK_EDITOR)
gs::EMap& gs::entityMap()
{
	return g_ctxImpl.entityMap;
}

Transform& gs::root()
{
	return g_ctxImpl.root;
}

void gs::detail::setup(Prop& out_prop)
{
	auto& transform = out_prop.transform();
	transform.parent(&g_ctxImpl.root);
	g_ctxImpl.entityMap[&transform] = out_prop.entity;
}
#endif
} // namespace le
