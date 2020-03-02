#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "core/io.hpp"
#include "core/jobs.hpp"
#include "core/log.hpp"
#include "core/maths.hpp"
#include "core/os.hpp"
#include "core/threads.hpp"
#include "core/map_store.hpp"

#include "engine/window.hpp"

int main(int argc, char** argv)
{
	using namespace le;

	os::Args args{argc, argv};
	os::init(args);
	{
		auto uLogger = log::logToFile("debug.log");
		auto const exeDirectory = os::dirPath(os::Dir::Executable);
		auto [result, dataPath] = FileReader::findUpwards(exeDirectory, {"data"});
		LOGIF_I(true, "Executable located at: {}", exeDirectory.generic_string());
		if (result == IOReader::Code::Success)
		{
			LOG_I("Found data at: {}", dataPath.generic_string());
		}
		else
		{
			LOG_E("Could not locate data!");
		}
		jobs::init(4);
		for (s32 i = 0; i < 10; ++i)
		{
			jobs::enqueue([i]() {
				std::this_thread::sleep_for(stdch::milliseconds(maths::randomRange(10, 1000)));
				LOG_I("Job #{}", i + 1);
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
		catch (std::exception const&)
		{
		}
		std::this_thread::sleep_for(stdch::milliseconds(maths::randomRange(10, 1000)));
		jobs::cleanup(true);
	}
	threads::joinAll();
	return 0;
}
