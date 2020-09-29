#pragma once
#include <memory>
#include <optional>
#include <glm/vec3.hpp>
#include <core/reader.hpp>
#include <engine/window/window.hpp>
#include <engine/resources/resource_types.hpp>

namespace le::engine
{
struct App
{
	std::optional<Window> window;
	io::FileReader fileReader;
	io::Reader const* pReader = nullptr;
};

inline glm::vec3 g_uiSpace = {};

res::Texture::Space colourSpace();
Window* window();

void update();
} // namespace le::engine
