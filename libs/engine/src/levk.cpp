#include <glm/gtx/quaternion.hpp>
#include "core/io.hpp"
#include "core/jobs.hpp"
#include "core/log.hpp"
#include "core/maths.hpp"
#include "core/map_store.hpp"
#include "core/services.hpp"
#include "engine/levk.hpp"
#include "engine/window.hpp"
#include "engine/vk/instance.hpp"

namespace le
{
s32 engine::run(s32 argc, char** argv)
{
	Services services;
	try
	{
		services.add<os::Service, os::Args>({argc, argv});
		services.add<log::Service, std::string_view>("debug.log");
		services.add<jobs::Service, u8>(4);
		services.add<Window::Service>();
		services.add<vuk::Instance::Service>();
	}
	catch (std::exception const& e)
	{
		LOG_E("Failed to initialise services: {}", e.what());
		return 1;
	}

	auto const exeDirectory = os::dirPath(os::Dir::Executable);
	auto [result, dataPath] = FileReader::findUpwards(exeDirectory, {"data"});
	LOGIF_I(true, "Executable located at: {}", exeDirectory.generic_string());
	if (result == IOReader::Code::Success)
	{
		LOG_D("Found data at: {}", dataPath.generic_string());
	}
	else
	{
		LOG_E("Could not locate data!");
		return 1;
	}
	for (s32 i = 0; i < 10; ++i)
	{
		jobs::enqueue([i]() {
			std::this_thread::sleep_for(stdch::milliseconds(maths::randomRange(10, 1000)));
			log::fmtLog(log::Level::Debug, "Job #{}", __FILE__, __LINE__, i);
		});
	}
	try
	{
		Window window, w2;
		Window::Data data;
		data.size = {1280, 720};
		data.title = "LittleEngineVk Demo";
		auto data2 = data;
		data2.title += " 2";
		data2.position = {100, 100};
		bool bRecreate2 = false;
		auto token = window.registerInput([&w2, &bRecreate2](Key key, Action action, Mods mods) {
			if (!w2.isOpen() && key == Key::W && action == le::Action::RELEASE && mods & Mods::CONTROL)
			{
				bRecreate2 = true;
			}
		});
		if (window.create(data) && w2.create(data2))
		{
			while (window.isOpen() || w2.isOpen())
			{
				Window::pollEvents();
				std::this_thread::sleep_for(stdch::milliseconds(10));
				if (window.isClosing())
				{
					window.destroy();
				}
				if (w2.isClosing())
				{
					w2.destroy();
				}
				if (bRecreate2)
				{
					bRecreate2 = false;
					w2.create(data2);
				}
			}
		}
	}
	catch (std::exception const& e)
	{
		LOG_E("Exception!\n\t{}", e.what());
	}
	std::this_thread::sleep_for(stdch::milliseconds(maths::randomRange(10, 1000)));
	return 0;
}
} // namespace le
