#pragma once
#include <levk/imported_scene.hpp>
#include <levk/lights.hpp>

namespace levk {
class SceneInfo : public ImportedScene {
  public:
	std::string skybox{};
	std::optional<DirectionalLight> main_light{};
};
} // namespace levk
