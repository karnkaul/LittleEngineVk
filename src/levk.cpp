#include <core/jobs.hpp>
#include <core/log.hpp>
#include <core/os.hpp>
#include <core/time.hpp>
#include <core/utils.hpp>
#include <engine/levk.hpp>
#include <engine/assets/resources.hpp>
#include <engine/game/world.hpp>
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
	m_services.add<log::Service>(std::string_view("debug.log"));
	m_services.add<jobs::Service>(4);
}

Service::Service(Service&&) = default;
Service& Service::operator=(Service&&) = default;
Service::~Service()
{
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
		auto [found, bResult] = FileReader::findUpwards(path, Span<stdfs::path>(pattern.patterns));
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
		FileReader fileReader;
		IOReader* pReader = info.pReader ? info.pReader : &fileReader;
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
		g_app.pReader = pReader;
	}
	catch (std::exception const& e)
	{
		LOG_E("[{}] Failed to initialise engine services: {}", tName, e.what());
		return false;
	}
	return true;
}

bool Service::tick(Time dt) const
{
	gfx::deferred::update();
	update();
	gfx::ScreenRect gameRect;
#if defined(LEVK_EDITOR)
	gameRect = editor::tick(dt);
#endif
	return World::tick(dt, gameRect);
}

bool Service::submitScene() const
{
	return World::submitScene(g_app.uWindow->renderer());
}

Window* Service::mainWindow()
{
	return g_app.uWindow.get();
}
} // namespace engine

void engine::update()
{
	Resources::inst().update();
	WindowImpl::update();
	gfx::vram::update();
}

IOReader const& engine::reader()
{
	ASSERT(g_app.pReader, "IOReader is null!");
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
