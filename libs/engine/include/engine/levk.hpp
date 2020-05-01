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
	Service(s32 argc, char** argv);
	Service(Service&&);
	Service& operator=(Service&&);
	~Service();
};

stdfs::path exePath();
stdfs::path dataPath();
} // namespace le::engine
