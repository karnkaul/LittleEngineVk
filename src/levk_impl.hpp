#pragma once
#include <memory>
#include <optional>
#include <core/io/reader.hpp>
#include <core/ref.hpp>
#include <engine/resources/resource_types.hpp>
#include <engine/window/window.hpp>
#include <glm/vec3.hpp>

namespace le::engine {
struct App {
	std::optional<Window> window;
	gfx::Viewport viewport;
	io::FileReader fileReader;
	Ref<io::Reader const> reader = fileReader;
};

res::Texture::Space colourSpace();
Window* window();

void update();
} // namespace le::engine
