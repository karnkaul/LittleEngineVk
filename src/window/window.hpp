#pragma once
#include <memory>
#include "core/zero.hpp"

namespace  le
{
class Window final
{
public:
	using ID = TZero<s32, -1>;

private:
	std::unique_ptr<class WindowImpl> m_uImpl;
	ID m_id = ID::Null;

public:
	Window();
	Window(Window&&);
	Window& operator=(Window&&);
	~Window();

public:
	ID id() const;
};
} // namespace  le
