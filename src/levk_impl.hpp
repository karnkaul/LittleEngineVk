#pragma once
#include <memory>
#include <engine/gfx/texture.hpp>
#include <engine/window/window.hpp>

namespace le::engine
{
struct App
{
	std::unique_ptr<Window> uWindow;
};

gfx::Texture::Space colourSpace();
void update();
} // namespace le::engine
