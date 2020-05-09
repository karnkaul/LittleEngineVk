#include "core/jobs.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/levk.hpp"
#include "engine/assets/resources.hpp"
#include "gfx/deferred.hpp"
#include "gfx/info.hpp"
#include "window/window_impl.hpp"

namespace le
{
namespace
{
stdfs::path g_exePath;
stdfs::path g_dataPath;
} // namespace

namespace engine
{
Service::Service() = default;
Service::Service(Service&&) = default;
Service& Service::operator=(Service&&) = default;
Service::~Service()
{
	g_exePath.clear();
	g_dataPath.clear();
}

bool Service::start(s32 argc, char** argv)
{
	g_exePath = argv[0];
	auto [dataPath, bResult] = FileReader::findUpwards(g_exePath.parent_path(), {"data"});
	if (!bResult)
	{
		LOG_E("[{}] Could not locate data!", utils::tName<Service>());
		return false;
	}
	g_dataPath = std::move(dataPath);
	try
	{
		m_services.add<os::Service, os::Args>({argc, argv});
		m_services.add<log::Service, std::string_view>("debug.log");
		m_services.add<jobs::Service, u8>(4);
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
		m_services.add<Resources::Service>();
	}
	catch (std::exception const& e)
	{
		LOG_E("[{}] Failed to initialise engine services: {}", utils::tName<Service>(), e.what());
		return false;
	}
	return true;
}

void Service::update()
{
	gfx::deferred::update();
	g_pResources->update();
	WindowImpl::updateActive();
}
} // namespace engine

stdfs::path engine::exePath()
{
	return g_exePath;
}

stdfs::path engine::dataPath()
{
	return g_dataPath;
}
} // namespace le
