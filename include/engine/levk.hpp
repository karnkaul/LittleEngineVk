#pragma once
#include <optional>
#include <vector>
#include <core/time.hpp>
#include <core/io.hpp>
#include <core/std_types.hpp>
#include <core/services.hpp>
#include <core/os.hpp>
#include <engine/gfx/screen_rect.hpp>
#include <engine/window/window.hpp>

namespace le::engine
{
struct DataSearch final
{
	std::vector<stdfs::path> patterns;
	os::Dir dirType = os::Dir::eExecutable;
};

struct Info final
{
	std::optional<Window::Info> windowInfo;
	std::vector<stdfs::path> dataPaths;
	IOReader* pReader = nullptr;
};

class Service final
{
private:
	Services m_services;

public:
	Service(s32 argc, char const* const* const argv);
	Service(Service&&);
	Service& operator=(Service&&);
	~Service();

	///
	/// \brief Locate data files for engine and/or app by searching upwards from the executable/working directory path
	/// \param searchPatterns List of directory / ZIP name patterns and start directories (executable / working) to search for (can include
	/// "/") \returns Fully qualified paths for each found pattern
	///
	std::vector<stdfs::path> locateData(std::vector<DataSearch> const& searchPatterns);

	bool init(Info const& info = {});
	bool tick(Time dt) const;
	bool submitScene() const;

	static Window* mainWindow();
};

stdfs::path exePath();
} // namespace le::engine
