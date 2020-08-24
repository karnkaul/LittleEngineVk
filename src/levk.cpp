#include <build_version.hpp>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <core/os.hpp>
#include <core/tasks.hpp>
#include <core/time.hpp>
#include <core/utils.hpp>
#include <engine/levk.hpp>
#include <engine/game/world.hpp>
#include <game/input_impl.hpp>
#include <gfx/deferred.hpp>
#include <gfx/device.hpp>
#include <gfx/ext_gui.hpp>
#include <gfx/renderer_impl.hpp>
#include <gfx/vram.hpp>
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
std::string const tName = utils::tName<Service>();
App g_app;
Status g_status = Status::eIdle;
} // namespace

Service::Service(s32 argc, char const* const* const argv)
{
	Time::resetElapsed();
	m_services.add<os::Service>(os::Args{argc, argv});
	m_services.add<io::Service>(std::string_view("debug.log"));
	LOG_I("LittleEngineVk {}", g_engineVersion.toString(false));
	m_services.add<tasks::Service>(4);
}

Service::Service(Service&&) = default;
Service& Service::operator=(Service&&) = default;
Service::~Service()
{
	input::deinit();
	World::impl_stopActive();
	res::waitIdle();
	World::impl_destroyAll();
	res::deinit();
	g_app = {};
}

std::vector<stdfs::path> Service::locateData(std::vector<DataSearch> const& searchPatterns)
{
	std::vector<stdfs::path> ret;
	auto const exe = os::dirPath(os::Dir::eExecutable);
	auto const pwd = os::dirPath(os::Dir::eWorking);
	for (auto const& pattern : searchPatterns)
	{
		auto const& path = pattern.dirType == os::Dir::eWorking ? pwd : exe;
		auto search = io::FileReader::findUpwards(path, Span<stdfs::path>(pattern.patterns));
		LOGIF_W(!search, "[{}] Failed to locate data!", tName);
		if (search)
		{
			ret.push_back(std::move(*search));
		}
	}
	return ret;
}

bool Service::init(Info const& info)
{
	try
	{
		m_services.add<Window::Service>();
		NativeWindow dummyWindow({});
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
		m_services.add<gfx::Service>(std::move(initInfo));
		auto const dirPath = os::dirPath(os::isDebuggerAttached() ? os::Dir::eWorking : os::Dir::eExecutable);
		io::FileReader fileReader;
		io::Reader* pReader = info.pReader ? info.pReader : &fileReader;
		std::vector<stdfs::path> const defaultPaths = {dirPath / "data"};
		auto const& dataPaths = info.dataPaths.empty() ? defaultPaths : info.dataPaths;
		for (auto const& path : dataPaths)
		{
			if (!pReader->mount(path))
			{
				throw std::runtime_error("Failed to mount data path" + path.generic_string() + "!");
			}
		}
		g_app.pReader = pReader;
		m_services.add<res::Service>();
		if (info.windowInfo)
		{
			g_app.uWindow = std::make_unique<Window>();
			if (!g_app.uWindow->create(*info.windowInfo))
			{
				throw std::runtime_error("Failed to create Window!");
			}
		}
		input::init(*g_app.uWindow);
	}
	catch (std::exception const& e)
	{
		g_app.pReader = nullptr;
		LOG_E("[{}] Failed to initialise engine services: {}", tName, e.what());
		return false;
	}
	g_status = Status::eInitialised;
	return true;
}

bool Service::start(World::ID world)
{
	if (g_status == Status::eInitialised && World::impl_startID(world))
	{
		g_status = Status::eTicking;
		return true;
	}
	return false;
}

bool Service::isRunning() const
{
	return maths::withinRange(g_status, Status::eIdle, Status::eShutdown, false);
}

Status Service::status() const
{
	return g_status;
}

bool Service::tick(Time dt) const
{
	Window::pollEvents();
	update();
	if (g_app.uWindow && g_app.uWindow->isClosing())
	{
		if (g_shutdownSequence == ShutdownSequence::eCloseWindow_Shutdown)
		{
			g_app.uWindow->destroy();
		}
		g_status = Status::eShuttingDown;
	}
	gfx::ScreenRect gameRect = {};
	bool bFireInput = true;
	bool bTickActive = true;
	bool bTerminate = false;
	bool bRet = true;
	switch (g_status)
	{
	default:
	{
		break;
	}
	case Status::eShutdown:
	{
		bTickActive = false;
		bFireInput = false;
		bTerminate = true;
		bRet = false;
		break;
	}
	case Status::eShuttingDown:
	{
		bTickActive = false;
		bFireInput = false;
		bTerminate = true;
		bRet = false;
		if (!World::isBusy())
		{
			doShutdown();
		}
		break;
	}
	}
	if (bFireInput)
	{
		input::fire();
#if defined(LEVK_EDITOR)
		editor::tick(dt);
		input::g_bEditorOnly = !editor::g_bTickGame;
		bTickActive &= editor::g_bTickGame;
		gameRect = editor::g_gameRect;
#endif
	}
	if (!World::impl_tick(dt, gameRect, bTickActive, bTerminate))
	{
		LOGIF_E(g_status == Status::eTicking, "[{}] Failed to tick World!", tName);
	}
	return bRet;
}

void Service::submitScene() const
{
	if (!isShuttingDown() && g_app.uWindow && World::s_pActive)
	{
		gfx::Camera const* pCamera = World::s_pActive->sceneCamPtr();
#if defined(LEVK_EDITOR)
		if (!editor::g_bTickGame)
		{
			pCamera = &editor::g_editorCam;
		}
#endif
		ASSERT(pCamera, "Camera is null!");
		if (!World::impl_submitScene(g_app.uWindow->renderer(), *pCamera))
		{
			LOG_E("[{}] Error submitting World scene!", tName);
		}
	}
}

void Service::render() const
{
	if (!isShuttingDown())
	{
		Window::renderAll();
	}
}

bool Service::shutdown()
{
	if (g_app.uWindow && g_app.uWindow->exists() && g_status == Status::eTicking)
	{
		g_app.uWindow->setClosing();
		if (g_shutdownSequence == ShutdownSequence::eCloseWindow_Shutdown)
		{
			g_app.uWindow->destroy();
		}
		g_status = Status::eShuttingDown;
		return true;
	}
	return false;
}

void Service::doShutdown()
{
	World::impl_destroyAll();
	g_app.uWindow->destroy();
	g_status = Status::eShutdown;
}
} // namespace engine

bool engine::isShuttingDown()
{
	return (g_app.uWindow && g_app.uWindow->isClosing()) || g_status == Status::eShuttingDown;
}

Window* engine::mainWindow()
{
	return g_app.uWindow.get();
}

glm::ivec2 engine::windowSize()
{
	return g_app.uWindow ? g_app.uWindow->windowSize() : glm::ivec2(0);
}

glm::ivec2 engine::framebufferSize()
{
	return g_app.uWindow ? g_app.uWindow->framebufferSize() : glm::ivec2(0);
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
	return g_app.uWindow.get();
}

io::Reader const& engine::reader()
{
	ASSERT(g_app.pReader, "io::Reader is null!");
	return *g_app.pReader;
}

res::Texture::Space engine::colourSpace()
{
	if (g_app.uWindow)
	{
		auto const pRenderer = WindowImpl::rendererImpl(g_app.uWindow->id());
		if (pRenderer && pRenderer->colourSpace() == ColourSpace::eSRGBNonLinear)
		{
			return res::Texture::Space::eSRGBNonLinear;
		}
	}
	return res::Texture::Space::eRGBLinear;
}
} // namespace le
