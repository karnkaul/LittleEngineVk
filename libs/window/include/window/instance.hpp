#pragma once
#include <memory>
#include <core/lib_logger.hpp>
#include <core/std_types.hpp>
#include <window/event_queue.hpp>

namespace le::window {
struct CreateInfo;
class Instance;

class Manager {
  public:
	explicit Manager();
	~Manager();

	bool ready() const noexcept { return m_impl != nullptr; }
	explicit operator bool() const noexcept { return ready(); }

	std::optional<Instance> make(CreateInfo const& info);

	std::size_t displayCount() const;

	class Impl;

  private:
	std::unique_ptr<Impl> m_impl;
	LibLogger m_logger;
};

enum class Style { eDecoratedWindow = 0, eBorderlessWindow, eBorderlessFullscreen, eDedicatedFullscreen, eCOUNT_ };
constexpr EnumArray<Style, std::string_view> const styleNames = {"Decorated Window", "Borderless Window", "Borderless Fullscreen", "Dedicated Fullscreen"};

struct CreateInfo {
	struct {
		std::string title;
		glm::ivec2 size = {640, 360};
		glm::ivec2 centreOffset = {};
	} config;

	struct {
		LibLogger::Verbosity verbosity = LibLogger::Verbosity::eLibUser;
		Style style = Style::eDecoratedWindow;
		u8 screenID = 0;
		bool centreCursor = true;
		bool autoShow = true;
	} options;
};

class Instance {
  public:
	Instance(Instance&&);
	Instance& operator=(Instance&&);
	~Instance();

	EventQueue pollEvents();
	bool show();
	bool hide();
	bool visible() const noexcept;
	void close();
	bool closing() const noexcept;

	glm::uvec2 framebufferSize() const noexcept;
	glm::uvec2 windowSize() const noexcept;
	CursorType cursorType() const noexcept;
	CursorMode cursorMode() const noexcept;
	glm::vec2 cursorPosition() const noexcept;
	void cursorType(CursorType type);
	void cursorMode(CursorMode mode);
	void cursorPosition(glm::vec2 position);

	bool importControllerDB(std::string_view db) const;
	ktl::fixed_vector<Gamepad, 8> activeGamepads() const;
	Joystick joyState(s32 id) const;
	Gamepad gamepadState(s32 id) const;
	std::size_t joystickAxesCount(s32 id) const;
	std::size_t joysticKButtonsCount(s32 id) const;

	static Key parseKey(std::string_view str) noexcept;
	static Action parseAction(std::string_view str) noexcept;
	static Mods parseMods(Span<std::string const> vec) noexcept;
	static Axis parseAxis(std::string_view str) noexcept;

	class Impl;

  private:
	Instance(std::unique_ptr<Impl>&& impl) noexcept;

	LibLogger m_log;
	std::unique_ptr<Impl> m_impl;

	friend class Manager;
	friend Impl& impl(Instance const&);
};
} // namespace le::window
