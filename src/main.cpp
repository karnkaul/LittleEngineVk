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

#include "window/window.hpp"

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
			Window window;
			Window::Data data;
			data.size = {1280, 720};
			data.title = "LittleEngineVk Demo";
			if (window.create(data))
			{
				while (window.isOpen())
				{
					window.pollEvents();
					std::this_thread::sleep_for(stdch::milliseconds(maths::randomRange(10, 1000)));
				}
			}
		}
		catch(std::exception const&)
		{
			
		}
		std::this_thread::sleep_for(stdch::milliseconds(maths::randomRange(10, 1000)));
		jobs::cleanup(true);
	}
	threads::joinAll();
	return 0;
}
