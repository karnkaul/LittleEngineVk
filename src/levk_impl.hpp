#pragma once
#include <memory>
#include <engine/window/window.hpp>

namespace le
{
struct App
{
	std::unique_ptr<Window> uWindow;
};
} // namespace le
