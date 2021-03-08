#include <core/ensure.hpp>
#include <core/log.hpp>
#include <dumb_json/djson.hpp>
#include <editor/editor.hpp>
#include <engine/game/driver.hpp>
#include <engine/game/state.hpp>
#include <engine/game/stopwatch.hpp>
#include <game/state_impl.hpp>
#include <resources/manifest.hpp>

namespace le {
namespace {
struct {
	std::optional<engine::Semaphore> engineSemaphore;
	res::Manifest manifest;
	gs::ManifestLoaded onManifestLoaded;
} g_ctxImpl;
} // namespace

Token gs::registerInput(input::Context const* pContext) {
	ENSURE(pContext, "Context is null!");
	return input::registerContext(pContext);
}

gs::Result<Token> gs::loadManifest(LoadReq const& loadReq) {
	if (!g_ctxImpl.manifest.idle()) {
		return gs::Result<Token>("[GameState] Manifest load already in progress!");
	}
	res::ResourceList unload;
	Result<Token> ret;
	if (!loadReq.unload.empty() && engine::reader().checkPresence(loadReq.unload)) {
		res::Manifest temp;
		temp.read(loadReq.unload);
		unload = temp.parse();
	}
	if (!loadReq.load.empty() && engine::reader().checkPresence(loadReq.load)) {
		g_ctxImpl.manifest.read(loadReq.load);
		auto const loadList = g_ctxImpl.manifest.parse();
		if (!loadList.empty()) {
			logI("[GameState] Loading manifest [{}] [{} resources]", loadReq.load.generic_string(), loadList.size());
			unload = unload - loadList;
			ret = g_ctxImpl.onManifestLoaded.subscribe(loadReq.onLoaded);
			g_ctxImpl.manifest.start();
			g_ctxImpl.engineSemaphore = engine::setBusy();
		} else {
			g_ctxImpl.manifest.reset();
		}
	}
	if (!unload.empty()) {
		logI("[GameState] Unloading manifest [{}] [{} resources]", loadReq.unload.generic_string(), unload.size());
		res::Manifest::unload(unload);
	}
	return ret;
}

Token gs::loadInputMap(io::Path const& id, input::Context* out_pContext) {
	if (!id.empty() && engine::reader().present(id)) {
		if (auto str = engine::reader().string(id)) {
			if (auto json = dj::node_t::make(*str)) {
				if (auto const parsed = out_pContext->deserialise(*json); parsed > 0) {
					logD("[GameState] Parsed [{}] input mappings from [{}]", parsed, id.generic_string());
				}
			} else {
				logW("[GameState] Failed to read input mappings from [{}]!", id.generic_string());
			}
		}
	}
	return registerInput(out_pContext);
}

void gs::reset() {
	g_game.reset();
	g_ctxImpl.manifest.reset();
	g_ctxImpl.onManifestLoaded.clear();
}

gfx::render::Driver::Scene gs::update(engine::Driver& out_driver, Time_s dt, bool bTick) {
	if (!g_ctxImpl.manifest.idle()) {
		auto const status = g_ctxImpl.manifest.update(!bTick);
		switch (status) {
		case res::Manifest::Status::eIdle: {
			g_ctxImpl.engineSemaphore = {};
			g_ctxImpl.manifest.reset();
			if (bTick) {
				g_ctxImpl.onManifestLoaded();
			}
			g_ctxImpl.onManifestLoaded.clear();
			break;
		}
		default: {
			break;
		}
		}
	}
	if (bTick) {
		out_driver.tick(dt);
		g_stopwatch.tick(dt);
	}
	Ref<gfx::Camera> camera = g_game.mainCamera();
#if defined(LEVK_EDITOR)
	if (!editor::g_bTickGame) {
		camera = editor::g_editorCam.m_camera;
	}
#endif
	return out_driver.builder().build(camera, g_game.m_registry);
}
} // namespace le
