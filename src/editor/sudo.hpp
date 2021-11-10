#pragma once
#include <engine/editor/scene_ref.hpp>
#include <utility>

namespace le::edi {
class Sudo {
  public:
	template <typename T, typename... Args>
	static void update(Args&&... args) {
		T::update(std::forward<Args>(args)...);
	}

	template <typename... T>
	static void clear() {
		(T::clear(), ...);
	}

	static dens::registry* registry(SceneRef scene) noexcept { return scene.m_registry; }
	static Inspect* inspect(SceneRef scene) noexcept { return scene.m_inspect; }
	static void inspect(SceneRef& out_scene, Inspect& out_ins) noexcept { out_scene.m_inspect = &out_ins; }
};
} // namespace le::edi
