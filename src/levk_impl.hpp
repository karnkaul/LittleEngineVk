#pragma once
#include <memory>
#include <glm/vec3.hpp>
#include <engine/gfx/texture.hpp>
#include <engine/window/window.hpp>

namespace le::engine
{
struct App
{
	std::unique_ptr<Window> uWindow;
	IOReader const* pReader = nullptr;
};

inline glm::vec3 g_uiSpace = {};

IOReader const& reader();
gfx::Texture::Space colourSpace();
void update();
Window* window();
} // namespace le::engine
