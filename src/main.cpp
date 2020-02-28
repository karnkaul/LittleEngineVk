#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace stdfs = std::filesystem;

struct DataLocation
{
	std::vector<stdfs::path> directories;
	std::vector<stdfs::path> files;
};

stdfs::path findData(stdfs::path const& path, DataLocation const& location)
{
	for (auto const& directory : location.directories)
	{
		if (std::filesystem::is_directory(path / directory))
		{
			return path;
		}
	}
	for (auto const& file : location.files)
	{
		if (std::filesystem::is_regular_file(file))
		{
			return path;
		}
	}
	return path.empty() || !path.has_parent_path() ? stdfs::path() : findData(path.parent_path(), location);
}

int main(int argc, char** argv)
{
	DataLocation location;
	auto exeDirectory = stdfs::path(argv[0]).parent_path();
	exeDirectory = std::filesystem::absolute(exeDirectory);
	location.directories = {"data"};
	auto dataPath = findData(exeDirectory, location);
	if (!dataPath.empty())
	{
		std::cout << "Found data at: " << dataPath.generic_string() << "\n";
	}
	else
	{
		std::cout << "Could not locate data!\n";
	}
	return 0;
}
