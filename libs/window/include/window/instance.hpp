#pragma once
#include <core/log_channel.hpp>
#include <core/std_types.hpp>
#include <window/event_queue.hpp>
#include <memory>
#include <optional>

namespace le::window {
struct CreateInfo;
class Instance;

class Manager {
  public:
	explicit Manager();
	~Manager() noexcept;

	bool ready() const noexcept { return m_impl != nullptr; }
	explicit operator bool() const noexcept { return ready(); }

	std::optional<Instance> make(CreateInfo const& info);

	std::size_t displayCount() const;

	struct Impl;

  private:
	std::unique_ptr<Impl> m_impl;
};

enum class Style { eDecoratedWindow = 0, eBorderlessWindow, eBorderlessFullscreen, eDedicatedFullscreen, eCOUNT_ };
constexpr EnumArray<Style, std::string_view> const styleNames = {"Decorated Window", "Borderless Window", "Borderless Fullscreen", "Dedicated Fullscreen"};

struct CreateInfo {
	struct {
		std::string title;
		glm::ivec2 size = {640, 360};
		std::optional<glm::ivec2> position;
		bool maximized = false;
	} config;

	struct {
		Style style = Style::eDecoratedWindow;
		u8 screenID = 0;
		bool centreCursor = true;
		bool autoShow = true;
	} options;
};

class Instance {
  public:
	Instance(Instance&&) noexcept;
	Instance& operator=(Instance&&) noexcept;
	~Instance() noexcept;

	EventQueue pollEvents();
	bool show();
	bool hide();
	bool visible() const noexcept;
	void close();
	bool closing() const noexcept;

	glm::ivec2 position() const noexcept;
	void position(glm::ivec2 pos) noexcept;
	bool maximized() const noexcept;
	void maximize() noexcept;
	void restore() noexcept;

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

	static std::string_view keyName(Key key, int scancode) noexcept;
	static Key parseKey(std::string_view str) noexcept;
	static Action parseAction(std::string_view str) noexcept;
	static Mods parseMods(Span<std::string const> vec) noexcept;
	static Axis parseAxis(std::string_view str) noexcept;

	class Impl;

  private:
	Instance(std::unique_ptr<Impl>&& impl) noexcept;

	std::unique_ptr<Impl> m_impl;

	friend class Manager;
	friend Impl& impl(Instance const&);
};
} // namespace le::window
