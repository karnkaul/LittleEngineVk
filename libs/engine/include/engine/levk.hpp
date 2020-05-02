#pragma once
#include <filesystem>
#include "core/std_types.hpp"
#include "core/services.hpp"

namespace stdfs = std::filesystem;

namespace le::engine
{
class Service final
{
private:
	Services m_services;

public:
	Service();
	Service(Service&&);
	Service& operator=(Service&&);
	~Service();

	bool start(s32 argc, char** argv);
};

stdfs::path exePath();
stdfs::path dataPath();
} // namespace le::engine
