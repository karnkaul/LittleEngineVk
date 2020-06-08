#include "core/jobs.hpp"
#include "core/log.hpp"
#include "core/time.hpp"
#include "core/utils.hpp"
#include "engine/levk.hpp"
#include "engine/assets/resources.hpp"
#include "gfx/deferred.hpp"
#include "gfx/device.hpp"
#include "gfx/ext_gui.hpp"
#include "gfx/vram.hpp"
#include "window/window_impl.hpp"

namespace le
{
namespace
{
stdfs::path g_exePath;
} // namespace

namespace engine
{
Service::Service(s32 argc, char* const* const argv)
{
	Time::resetElapsed();
	g_exePath = argv[0];
	m_services.add<os::Service>(os::Args{argc, argv});
	m_services.add<log::Service>(std::string_view("debug.log"));
	m_services.add<jobs::Service>(4);
}

Service::Service(Service&&) = default;
Service& Service::operator=(Service&&) = default;
Service::~Service()
{
	Resources::inst().deinit();
	g_exePath.clear();
}

bool Service::start(IOReader const& data)
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
		Resources::inst().init(data);
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
	gfx::vram::update();
	gfx::deferred::update();
	Resources::inst().update();
	WindowImpl::update();
}
} // namespace engine

stdfs::path engine::exePath()
{
	return g_exePath;
}
} // namespace le
