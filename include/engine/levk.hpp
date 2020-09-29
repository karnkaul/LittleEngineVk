#pragma once
#include <optional>
#include <vector>
#include <core/counter.hpp>
#include <core/time.hpp>
#include <core/reader.hpp>
#include <core/std_types.hpp>
#include <core/services.hpp>
#include <core/os.hpp>
#include <engine/gfx/screen_rect.hpp>
#include <engine/window/window.hpp>
#if defined(LEVK_DEBUG)
#include <core/log_config.hpp>
#endif

#if defined(LEVK_EDITOR)
constexpr bool levk_editor = true;
#else
constexpr bool levk_editor = false;
#endif

namespace le::engine
{
using Semaphore = TCounter<s32>::Semaphore;

struct Driver;

enum class Status : s8
{
	eIdle,
	eInitialised,
	eTicking,
	eShuttingDown,
	eShutdown
};

enum class ShutdownSequence : s8
{
	eCloseWindow_Shutdown,
	eShutdown_CloseWindow
};
inline ShutdownSequence g_shutdownSequence = ShutdownSequence::eCloseWindow_Shutdown;

struct MemRange final
{
	std::size_t size = 2;
	std::size_t count = 1;
};

struct Info final
{
	std::optional<Window::Info> windowInfo;
	io::Reader* pCustomReader = nullptr;
	Span<stdfs::path> dataPaths;
	Span<MemRange> vramReserve;
#if defined(LEVK_DEBUG)
	bool bLogVRAMallocations = false;
	io::Level vramLogLevel = io::Level::eDebug;
#endif
};

class Service final
{
public:
	Service(os::Args args = {});
	Service(Service&&);
	Service& operator=(Service&&);
	~Service();

	///
	/// \brief Initialise engine and dependent services
	/// \returns `false` if initialisation failed
	///
	bool init(Info info = {});
	///
	/// \brief Check whether engine is currently running (or shutting down)
	///
	bool running() const;
	///
	/// \brief Obtain current engine Status
	///
	Status status() const;
	///
	/// \brief Update all services, tick Driver and submit scene
	///
	bool update(Driver& out_driver) const;
	///
	/// \brief Render all active windows
	///
	void render() const;

	///
	/// \brief Shutdown engine and close main window
	///
	static bool shutdown();

private:
	static void doShutdown();

private:
	Services m_services;
};

///
/// \brief Locate files / directories on the filesysten by searching upwards from the executable/working directory path
/// \param patterns List of directory / filename patterns to search for
/// \param dirType Starting directory (executable / working)
/// \returns Fully qualified paths for each found pattern
///
std::vector<stdfs::path> locate(Span<stdfs::path> patterns, os::Dir dirType = os::Dir::eExecutable);

///
/// \brief Obtain whether the engine is shutting down
///
bool shuttingDown();
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
/// \brief Obtain the (normalised) gameRect size
///
glm::vec2 gameRectSize();
///
/// \brief Obtain the path to the running executable
///
stdfs::path exePath();
///
/// \brief Obtain the data reader
///
io::Reader const& reader();
///
/// \brief Obtain busy Semaphore
///
/// Prevents shutdown until all Semaphores are released
///
Semaphore setBusy();
///
/// \brief Check whether any Semaphore is active
///
bool busy();
} // namespace le::engine
