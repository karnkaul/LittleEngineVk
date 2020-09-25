#include <build_version.hpp>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <core/os.hpp>
#include <core/tasks.hpp>
#include <core/time.hpp>
#include <core/utils.hpp>
#include <dumb_json/dumb_json.hpp>
#include <engine/levk.hpp>
#include <engine/game/driver.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/game/state.hpp>
#include <game/input_impl.hpp>
#include <gfx/deferred.hpp>
#include <gfx/device.hpp>
#include <gfx/ext_gui.hpp>
#include <gfx/pipeline_impl.hpp>
#include <gfx/render_driver_impl.hpp>
#include <gfx/vram.hpp>
#include <game/state_impl.hpp>
#include <resources/resources_impl.hpp>
#include <window/window_impl.hpp>
#include <editor/editor.hpp>
#include <levk_impl.hpp>

namespace le
{
namespace engine
{
namespace
{
struct Clock
{
	Time stamp;

	Time dt() noexcept
	{
		Time now = Time::elapsed();
		Time ret = stamp == Time() ? Time() : now - stamp;
		stamp = now;
		return ret;
	}
};

std::string const tName = utils::tName<Service>();
App g_app;
Status g_status = Status::eIdle;
TCounter<s32> g_counter;
Clock g_clock;
} // namespace

Service::Service(os::Args args)
{
	dj::g_log_error = [](auto text) { LOG_E("{}", text); };
	Time::resetElapsed();
	m_services.add<os::Service>(args);
	m_services.add<io::Service>(std::string_view("debug.log"));
	LOG_I("LittleEngineVk v{}  [{}/{}]", g_engineVersion.toString(false), levk_OS_name, levk_arch_name);
	m_services.add<tasks::Service>(4);
}

Service::Service(Service&&) = default;
Service& Service::operator=(Service&&) = default;
Service::~Service()
{
	// Order is critical!
	// Disable all input
	input::deinit();
	// Wait for async loads
	res::waitIdle();
	// Reset game state
	gs::reset();
	// Release all pipelines and Shader semaphores
	gfx::pipes::deinit();
	// Release all resources
	res::deinit();
	g_app = {};
}

bool Service::init(Info const& info)
{
	try
	{
		m_services.add<Window::Service>();
		NativeWindow dummyWindow(Window::Info{});
		gfx::InitInfo initInfo;
#if defined(LEVK_DEBUG)
		gfx::g_VRAM_bLogAllocs = info.bLogVRAMallocations;
		gfx::g_VRAM_logLevel = info.vramLogLevel;
		initInfo.options.flags.set(gfx::InitInfo::Flag::eValidation);
#endif
		if (os::isDefined("validation"))
		{
			LOG_I("Validation layers requested, enabling...");
			initInfo.options.flags.set(gfx::InitInfo::Flag::eValidation);
		}
		initInfo.config.instanceExtensions = WindowImpl::vulkanInstanceExtensions();
		initInfo.config.createTempSurface = [&](vk::Instance instance) -> vk::SurfaceKHR { return dummyWindow.createSurface(instance); };
		initInfo.config.stagingReserve = info.vramReserve;
		m_services.add<gfx::Service>(std::move(initInfo));
		auto const dirPath = os::dirPath(os::isDebuggerAttached() ? os::Dir::eWorking : os::Dir::eExecutable);
		io::FileReader fileReader;
		io::Reader& reader = info.reader;
		std::vector<stdfs::path> const defaultPaths = {dirPath / "data"};
		auto const& dataPaths = info.dataPaths.empty() ? defaultPaths : info.dataPaths;
		for (auto const& path : dataPaths)
		{
			if (!reader.mount(path))
			{
				throw std::runtime_error("Failed to mount data path" + path.generic_string() + "!");
			}
		}
		g_app.pReader = &reader;
		m_services.add<res::Service>();
		Window::Info windowInfo;
		windowInfo.config.size = {1280, 720};
		g_app.window = Window();
		if (!g_app.window->create(info.windowInfo ? *info.windowInfo : windowInfo))
		{
			throw std::runtime_error("Failed to create Window!");
		}
		input::init(*g_app.window);
	}
	catch (std::exception const& e)
	{
		g_app.pReader = nullptr;
		LOG_E("[{}] Failed to initialise engine services: {}", tName, e.what());
		return false;
	}
	g_status = Status::eInitialised;
	g_clock = {};
	return true;
}

bool Service::running() const
{
	return maths::inRange(g_status, Status::eIdle, Status::eShuttingDown, true);
}

Status Service::status() const
{
	return g_status;
}

bool Service::update(Driver& out_driver) const
{
	if (g_status == Status::eIdle)
	{
		return false;
	}
	if (g_status == Status::eInitialised)
	{
		g_status = Status::eTicking;
	}
	Time const dt = g_clock.dt();
	Window::pollEvents();
	engine::update();
	if (g_app.window && g_app.window->closing())
	{
		if (g_shutdownSequence == ShutdownSequence::eCloseWindow_Shutdown)
		{
			g_app.window->destroy();
		}
		g_status = Status::eShuttingDown;
	}
	gfx::ScreenRect gameRect = {};
	bool bTerminating = g_status == Status::eShutdown || g_status == Status::eShuttingDown;
	if (g_status == Status::eShuttingDown && g_counter.isZero(true))
	{
		doShutdown();
	}
	bool bTick = g_status == Status::eTicking && !bTerminating;
	if (!bTerminating)
	{
		input::fire();
#if defined(LEVK_EDITOR)
		editor::tick(gs::g_game, dt);
		bool const bEditorOnly = !editor::g_bTickGame && !editor::g_bStepGame.get();
		input::g_bEditorOnly = bEditorOnly;
		bTick &= !bEditorOnly;
		gameRect = editor::g_gameRect;
#endif
	}
#if defined(LEVK_DEBUG)
	try
#endif
	{
		g_app.window->driver().submit(gs::update(out_driver, dt, bTick), gameRect);
	}
#if defined(LEVK_DEBUG)
	catch (std::exception const& e)
	{
		LOG_E("EXCEPTION!\n\t{}", e.what());
	}
#endif
	return !bTerminating;
}

void Service::render() const
{
	if (!shuttingDown())
	{
#if defined(LEVK_DEBUG)
		try
#endif
		{
			Window::renderAll();
		}
#if defined(LEVK_DEBUG)
		catch (std::exception const& e)
		{
			LOG_E("EXCEPTION!\n\t{}", e.what());
		}
#endif
	}
}

bool Service::shutdown()
{
	if (g_app.window && g_app.window->exists() && g_status == Status::eTicking)
	{
		g_app.window->setClosing();
		if (g_shutdownSequence == ShutdownSequence::eCloseWindow_Shutdown)
		{
			g_app.window->destroy();
		}
		g_status = Status::eShuttingDown;
		input::g_bFire = false;
		return true;
	}
	return false;
}

void Service::doShutdown()
{
	gs::reset();
	g_app.window->destroy();
	g_status = Status::eShutdown;
}
} // namespace engine

std::vector<stdfs::path> engine::locate(Span<stdfs::path> patterns, os::Dir dirType)
{
	std::vector<stdfs::path> ret;
	auto const start = os::dirPath(dirType);
	for (auto const& pattern : patterns)
	{
		auto search = io::FileReader::findUpwards(start, pattern);
		LOGIF_W(!search, "[{}] Failed to locate [{}] from [{}]!", tName, pattern.generic_string(), start.generic_string());
		if (search)
		{
			ret.push_back(std::move(*search));
		}
	}
	return ret;
}

bool engine::shuttingDown()
{
	return (g_app.window && g_app.window->closing()) || g_status == Status::eShuttingDown;
}

Window* engine::mainWindow()
{
	return engine::window();
}

glm::ivec2 engine::windowSize()
{
	return g_app.window ? g_app.window->windowSize() : glm::ivec2(0);
}

glm::ivec2 engine::framebufferSize()
{
	return g_app.window ? g_app.window->framebufferSize() : glm::ivec2(0);
}

glm::vec2 engine::gameRectSize()
{
#if defined(LEVK_EDITOR)
	return editor::g_gameRect.size();
#else
	return {1.0f, 1.0f};
#endif
}

void engine::update()
{
	res::update();
	WindowImpl::update();
	gfx::vram::update();
	gfx::deferred::update();
}

Window* engine::window()
{
	return g_app.window ? &(g_app.window.value()) : nullptr;
}

io::Reader const& engine::reader()
{
	ASSERT(g_app.pReader, "io::Reader is null!");
	return *g_app.pReader;
}

res::Texture::Space engine::colourSpace()
{
	if (g_app.window)
	{
		auto const pDriver = WindowImpl::driverImpl(g_app.window->id());
		if (pDriver && pDriver->colourSpace() == ColourSpace::eSRGBNonLinear)
		{
			return res::Texture::Space::eSRGBNonLinear;
		}
	}
	return res::Texture::Space::eRGBLinear;
}

engine::Semaphore engine::setBusy()
{
	return Semaphore(g_counter);
}

bool engine::busy()
{
	return !g_counter.isZero(true);
}
} // namespace le
