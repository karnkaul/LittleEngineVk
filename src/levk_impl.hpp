#pragma once
#include <memory>
#include <engine/gfx/texture.hpp>
#include <engine/window/window.hpp>

namespace le::engine
{
struct App
{
	std::unique_ptr<Window> uWindow;
	IOReader const* pReader = nullptr;
};

IOReader const& reader();
gfx::Texture::Space colourSpace();
void update();
Window* window();
} // namespace le::engine
