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
#include <engine/game/world.hpp>
#if defined(LEVK_DEBUG)
#include <core/log_config.hpp>
#endif

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
#if defined(LEVK_DEBUG)
	bool bLogVRAMallocations = false;
	log::Level vramLogLevel = log::Level::eDebug;
#endif
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
	/// \returns Fully qualified paths for each found pattern
	///
	std::vector<stdfs::path> locateData(std::vector<DataSearch> const& searchPatterns);
	///
	/// \brief Initialise engine and dependent services
	/// \returns `false` if initialisation failed
	///
	bool init(Info const& info = {});
	///
	/// \brief Start running the desired world
	/// \returns `false` if world does not exist / could not be started
	///
	bool start(World::ID world);
	///
	/// \brief Check whether any windows are active
	///
	bool isRunning() const;
	///
	/// \brief Update all services and tick active world
	/// \returns `false` if no world is active
	///
	bool tick(Time dt) const;
	///
	/// \brief Submit scene from active world
	/// \returns `false` if no world is active
	///
	bool submitScene() const;
	///
	/// \brief Render all active windows
	///
	void render() const;
};

///
/// \brief Start shutdown of main window
///
bool terminate();
///
/// \brief Obtain whether the engine is shutting down
///
bool isTerminating();
///
/// \brief Obtain the main window
///
Window* mainWindow();
///
/// \brief Obtain the window size
///
glm::ivec2 windowSize();
///
/// \brief Obtain the framebuffer size
///
glm::ivec2 framebufferSize();
///
/// \brief Obtain the path to the running executable
///
stdfs::path exePath();
} // namespace le::engine
