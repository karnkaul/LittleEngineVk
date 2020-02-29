#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include "core/os.hpp"
#include "core/file_logger.hpp"
#include "core/log.hpp"
#include "core/io.hpp"
#include "core/os.hpp"
#include "core/threads.hpp"
#include "core/map_store.hpp"

int main([[maybe_unused]] int argc, char** argv)
{
	using namespace le;

	os::Args args{argc, argv};
	os::init(args);
	{
		FileLogger fileLogger("debug.log");
		auto const exeDirectory = os::dirPath(os::Dir::Executable);
		auto [result, dataPath] = FileReader::findUpwards(exeDirectory, {"data"});
		LOG_I("Executable located at: %s", exeDirectory.generic_string().data());
		if (result == IOReader::Code::Success)
		{
			LOG_I("Found data at: %s", dataPath.generic_string().data());
		}
		else
		{
			LOG_E("Could not locate data!");
		}
	}
	return 0;
}
