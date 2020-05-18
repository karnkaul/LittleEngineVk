#pragma once
#include "core/io.hpp"
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
	Service(s32 argc, char* const* const argv);
	Service(Service&&);
	Service& operator=(Service&&);
	~Service();

	bool start(IOReader const& data);
	void update();
};

stdfs::path exePath();
} // namespace le::engine
