#pragma once
#include <le/graphics/font/codepoint.hpp>

namespace le::input {
using graphics::Codepoint;

class Receiver {
  public:
	Receiver(Receiver const&) = delete;
	Receiver(Receiver&&) = delete;
	auto operator=(Receiver const&) -> Receiver& = delete;
	auto operator=(Receiver&&) -> Receiver& = delete;

	Receiver();
	virtual ~Receiver();

	virtual auto on_key(int /*key*/, int /*action*/, int /*mods*/) -> bool { return false; }
	virtual auto on_mouse(int /*key*/, int /*action*/, int /*mods*/) -> bool { return false; }
	virtual auto on_char(Codepoint /*codepoint*/) -> bool { return false; }
};
} // namespace le::input
