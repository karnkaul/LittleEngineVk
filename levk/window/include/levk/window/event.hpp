#pragma once
#include <glm/vec2.hpp>
#include <levk/core/span.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/window/types.hpp>
#include <string>

namespace le::window {
class Event {
  public:
	enum class Type : std::uint8_t {
		eNone,
		eClosed,
		eFocusChange,
		eCursorEnter,
		eMaximize,
		eIconify,
		ePosition,
		eWindowResize,
		eFramebufferResize,
		eCursor,
		eScroll,
		eKey,
		eMouseButton,
		eText,
		eFileDrop,
	};

	struct Key {
		int scancode;
		int key;
		int action;
		int mods;
	};

	struct Button {
		int button;
		int action;
		int mods;
	};

	Type type() const noexcept { return m_type; }

	bool closed() const noexcept { return (check(Type::eClosed), true); }
	bool focusGained() const noexcept { return (check(Type::eFocusChange), m_payload.b); }
	bool cursorEntered() const noexcept { return (check(Type::eCursorEnter), m_payload.b); }
	bool maximized() const noexcept { return (check(Type::eMaximize), m_payload.b); }
	bool iconified() const noexcept { return (check(Type::eIconify), m_payload.b); }
	glm::ivec2 positioned() const noexcept { return (check(Type::ePosition), iv2()); }
	glm::uvec2 framebufferSize() const noexcept { return (check(Type::eFramebufferResize), uv2()); }
	glm::uvec2 windowSize() const noexcept { return (check(Type::eWindowResize), uv2()); }
	glm::dvec2 cursor() const noexcept { return (check(Type::eCursor), dv2()); }
	glm::dvec2 scroll() const noexcept { return (check(Type::eScroll), dv2()); }
	Key const& key() const noexcept { return (check(Type::eKey), m_payload.key); }
	Button const& mouseButton() const noexcept { return (check(Type::eMouseButton), m_payload.button); }
	std::uint32_t codepoint() const noexcept { return (check(Type::eText), m_payload.u32); }
	Span<std::string const> fileDrop() const noexcept;

	struct Builder;

  private:
	void check(Type const type) const noexcept { EXPECT(m_type == type); }
	glm::uvec2 uv2() const noexcept { return {m_payload.uv2.x, m_payload.uv2.y}; }
	glm::ivec2 iv2() const noexcept { return {m_payload.iv2.x, m_payload.iv2.y}; }
	glm::dvec2 dv2() const noexcept { return {m_payload.dv2.x, m_payload.dv2.y}; }

	template <typename T>
	struct v2 {
		T x;
		T y;
	};

	union {
		Key key;
		Button button;
		v2<std::int32_t> iv2;
		v2<std::uint32_t> uv2;
		v2<double> dv2;
		std::uint32_t u32;
		bool b;
		std::size_t index;
	} m_payload;
	Type m_type = Type::eNone;
};
} // namespace le::window
