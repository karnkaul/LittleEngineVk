#include <core/log.hpp>
#include <engine/game/freecam.hpp>
#include <engine/game/state.hpp>
#include <engine/levk.hpp>
#include <level.hpp>
#include <levels/demo.hpp>
#include <levels/test.hpp>

using namespace le;

namespace
{
struct Switcher final
{
	using Make = std::unique_ptr<Level> (*)();

	Make makePtr = nullptr;

	template <typename T>
	void schedule()
	{
		makePtr = []() -> std::unique_ptr<Level> { return std::make_unique<T>(); };
	}

	std::unique_ptr<Level> make()
	{
		std::unique_ptr<Level> ret;
		if (makePtr)
		{
			ret = makePtr();
			clear();
		}
		return ret;
	}

	void clear()
	{
		makePtr = nullptr;
	}

	bool hasValue() const
	{
		return makePtr != nullptr;
	}
};

Switcher g_switcher;
} // namespace

Level::Level() = default;
Level::~Level() = default;

SceneBuilder const& Level::builder() const
{
	static SceneBuilder const s_default;
	return s_default;
}

void Level::onManifestLoaded() {}

Registry const& Level::registry() const
{
	return gs::g_context.registry;
}

Registry& Level::registry()
{
	return gs::g_context.registry;
}

gfx::Camera const& Level::camera() const
{
	return gs::g_context.camera;
}

gfx::Camera& Level::camera()
{
	return gs::g_context.camera;
}

gfx::ScreenRect const& Level::gameRect() const
{
	return gs::g_context.gameRect;
}

gfx::ScreenRect& Level::gameRect()
{
	return gs::g_context.gameRect;
}

void LevelDriver::tick(Time dt)
{
	if (!m_bTicked)
	{
		m_bTicked = true;
		g_switcher.schedule<DemoLevel>();
	}
	if (g_switcher.hasValue())
	{
		Registry& reg = gs::g_context.registry;
		reg.clear();
		unloadLoad();
		return;
	}
	if (m_active)
	{
		m_active->tick(dt);
	}
	if (levk_editor)
	{
		perFrame();
	}
	return;
}

SceneBuilder const& LevelDriver::builder() const
{
	return m_active ? m_active->builder() : engine::Driver::builder();
}

void LevelDriver::cleanup()
{
	m_bTicked = false;
	g_switcher.clear();
	m_active.reset();
	Registry& reg = gs::g_context.registry;
	reg.clear();
	gs::g_context.camera = {};
	gs::g_context.gameRect = {};
}

void LevelDriver::unloadLoad()
{
	gs::LoadReq loadReq;
	if (m_active)
	{
		loadReq.unload = m_active->m_manifest.id;
	}
	m_active = g_switcher.make();
	loadReq.load = m_active->m_manifest.id;
	loadReq.onLoaded = [l = loadReq.load.generic_string(), this]() {
		LOG_I("[{}] loaded!", l);
		m_active->onManifestLoaded();
	};
	if (auto token = gs::loadManifest(loadReq))
	{
		m_active->m_manifest.token = std::move(*token);
	}
	m_active->m_input.token = gs::loadInputMap(m_active->m_input.id, &m_active->m_input.context);
}

void LevelDriver::perFrame()
{
#if defined(LEVK_EDITOR)
	static constexpr std::array<std::string_view, 2> levelNames = {"Demo", "Test"};
	gs::g_context.editorData.customRightPanel = [this]() {
		if (auto combo = editor::Combo("Level", levelNames, m_active->m_name))
		{
			if (m_active && combo.selected != m_active->m_name)
			{
				if (combo.selected == levelNames.at(0))
				{
					g_switcher.schedule<DemoLevel>();
				}
				else if (combo.selected == levelNames.at(1))
				{
					g_switcher.schedule<TestLevel>();
				}
			}
		}
	};
#endif
}
