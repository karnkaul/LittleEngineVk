#pragma once
#include <levk/engine/engine.hpp>
#include <levk/graphics/device/device.hpp>
#include <levk/graphics/device/vram.hpp>
#include <levk/graphics/render/vsync.hpp>
#include <levk/window/window.hpp>

namespace le {
struct Engine::CustomDevice {
	std::string name;
};

struct Engine::CreateInfo {
	window::CreateInfo winInfo;
	std::optional<io::Path> logFile = "log.txt";
	LogChannel logChannels = log_channels_v;
};

struct Engine::BootInfo {
	Device::CreateInfo device;
	VRAM::CreateInfo vram;
	std::optional<graphics::VSync> vsync;
};

class Engine::Builder {
  public:
	static Span<graphics::PhysicalDevice const> availableDevices();

	Builder& window(window::CreateInfo info) noexcept { return (m_windowInfo = std::move(info), *this); }
	Builder& logFile(io::Path path) noexcept { return (m_logFile = std::move(path), *this); }
	Builder& noLogFile() noexcept { return (m_logFile.reset(), *this); }
	Builder& logChannels(LogChannel const lc) noexcept { return (m_logChannels = lc, *this); }
	Builder& media(not_null<io::Media const*> m) noexcept { return (m_media = m, *this); }
	Builder& configFile(io::Path path) noexcept { return (m_configPath = std::move(path), *this); }
	Builder& addIcon(io::Path uri) noexcept { return (m_iconURIs.push_back(std::move(uri)), *this); }

	std::optional<Engine> operator()();

  private:
	window::CreateInfo m_windowInfo;
	std::optional<io::Path> m_logFile = "log.txt";
	std::vector<io::Path> m_iconURIs;
	io::Path m_configPath = "levk_config.json";
	LogChannel m_logChannels = log_channels_v;
	io::Media const* m_media{};
};
} // namespace le
