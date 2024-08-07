#pragma once
#include <levk/imported_scene.hpp>
#include <levk/lights.hpp>

namespace levk {
struct SceneInfo : ImportedScene {
	std::string skybox{};
	std::optional<DirectionalLight> main_light{};
};
} // namespace levk
