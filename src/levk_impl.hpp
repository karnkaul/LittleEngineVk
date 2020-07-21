#pragma once
#include <memory>
#include <glm/vec3.hpp>
#include <core/reader.hpp>
#include <engine/window/window.hpp>
#include <engine/resources/resource_types.hpp>

namespace le::engine
{
struct App
{
	std::unique_ptr<Window> uWindow;
	io::Reader const* pReader = nullptr;
};

inline glm::vec3 g_uiSpace = {};

io::Reader const& reader();
res::Texture::Space colourSpace();
Window* window();

void update();
void destroyWindow();
} // namespace le::engine
