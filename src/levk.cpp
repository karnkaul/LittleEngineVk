#include <core/log.hpp>
#include <core/os.hpp>
#include <core/tasks.hpp>
#include <core/time.hpp>
#include <core/utils.hpp>
#include <engine/levk.hpp>
#include <engine/assets/resources.hpp>
#include <engine/game/world.hpp>
#include <game/input_impl.hpp>
#include <gfx/deferred.hpp>
#include <gfx/device.hpp>
#include <gfx/ext_gui.hpp>
#include <gfx/renderer_impl.hpp>
#include <gfx/vram.hpp>
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
} // namespace

Service::Service(s32 argc, char const* const* const argv)
{
	Time::resetElapsed();
	m_services.add<os::Service>(os::Args{argc, argv});
	m_services.add<io::Service>(std::string_view("debug.log"));
	m_services.add<tasks::Service>(4);
}

Service::Service(Service&&) = default;
Service& Service::operator=(Service&&) = default;
Service::~Service()
{
	input::deinit();
	World::stopActive();
	Resources::inst().waitIdle();
	World::destroyAll();
	Resources::inst().deinit();
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
		auto [found, bResult] = io::FileReader::findUpwards(path, Span<stdfs::path>(pattern.patterns));
		LOGIF_W(!bResult, "[{}] Failed to locate data!", tName);
		if (bResult)
		{
			ret.push_back(std::move(found));
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
		initInfo.config.createTempSurface = [&](vk::Instance instance) { return WindowImpl::createSurface(instance, dummyWindow); };
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
		Resources::inst().init(*pReader);
		if (info.windowInfo)
		{
			g_app.uWindow = std::make_unique<Window>();
			if (!g_app.uWindow->create(*info.windowInfo))
			{
				throw std::runtime_error("Failed to create Window!");
			}
		}
		input::init(*g_app.uWindow);
		g_app.pReader = pReader;
	}
	catch (std::exception const& e)
	{
		LOG_E("[{}] Failed to initialise engine services: {}", tName, e.what());
		return false;
	}
	return true;
}

bool Service::start(World::ID world)
{
	return World::start(world);
}

bool Service::isRunning() const
{
	return Window::anyExist();
}

bool Service::tick(Time dt) const
{
	Window::pollEvents();
	update();
	bool const bShutdown = isShuttingDown();
	if (bShutdown)
	{
		shutdown();
	}
	else
	{
		gfx::ScreenRect gameRect = {};
#if defined(LEVK_EDITOR)
		if (editor::g_bTickGame)
		{
			input::fire();
		}
		editor::tick(dt);
		gameRect = editor::g_gameRect;
#else
		input::fire();
#endif
		if (!World::tick(dt, gameRect))
		{
			LOG_E("[{}] Failed to tick World!", tName);
		}
	}
	return !bShutdown;
}

void Service::submitScene() const
{
	if (!isShuttingDown() && g_app.uWindow)
	{
		if (!World::submitScene(g_app.uWindow->renderer()))
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
	if (g_app.uWindow && g_app.uWindow->exists())
	{
		World::destroyAll();
		g_app.uWindow->close();
		destroyWindow();
		return true;
	}
	return false;
}
} // namespace engine

bool engine::isShuttingDown()
{
	return g_app.uWindow && g_app.uWindow->isClosing();
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

void engine::update()
{
	Resources::inst().update();
	WindowImpl::update();
	gfx::vram::update();
	gfx::deferred::update();
}

void engine::destroyWindow()
{
	if (g_app.uWindow)
	{
		g_app.uWindow->destroy();
	}
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

gfx::Texture::Space engine::colourSpace()
{
	if (g_app.uWindow)
	{
		auto const pRenderer = WindowImpl::rendererImpl(g_app.uWindow->id());
		if (pRenderer && pRenderer->colourSpace() == ColourSpace::eSRGBNonLinear)
		{
			return gfx::Texture::Space::eSRGBNonLinear;
		}
	}
	return gfx::Texture::Space::eRGBLinear;
}
} // namespace le
