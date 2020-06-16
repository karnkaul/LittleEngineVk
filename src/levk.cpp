#include <core/jobs.hpp>
#include <core/log.hpp>
#include <core/time.hpp>
#include <core/utils.hpp>
#include <engine/levk.hpp>
#include <engine/assets/resources.hpp>
#include <gfx/deferred.hpp>
#include <gfx/device.hpp>
#include <gfx/ext_gui.hpp>
#include <gfx/vram.hpp>
#include <levk_impl.hpp>
#include <window/window_impl.hpp>
#include <core/os.hpp>

namespace le::engine
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
	}
	catch (std::exception const& e)
	{
		LOG_E("[{}] Failed to initialise engine services: {}", tName, e.what());
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
} // namespace le::engine
