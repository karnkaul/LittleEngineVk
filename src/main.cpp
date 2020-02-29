#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "core/jobs.hpp"
#include "core/os.hpp"
#include "core/log.hpp"
#include "core/io.hpp"
#include "core/os.hpp"
#include "core/threads.hpp"
#include "core/map_store.hpp"

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
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		jobs::cleanup();
	}
	threads::joinAll();
	return 0;
}
