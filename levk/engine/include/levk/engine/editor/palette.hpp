#pragma once
#include <levk/core/utils/expect.hpp>
#include <levk/core/utils/vbase.hpp>
#include <levk/engine/editor/types.hpp>

namespace le::editor {
class PaletteTab;

class Palette : public utils::VBase {
  public:
	bool hasScene() const noexcept { return m_scene; }
	SceneRef scene() const;

	virtual void update() = 0;

  private:
	void doUpdate(SceneRef scene) {
		m_scene = &scene;
		update();
	}

	SceneRef const* m_scene{};

	friend class PaletteTab;
};

// impl

inline SceneRef Palette::scene() const {
	EXPECT(m_scene);
	return *m_scene;
}
} // namespace le::editor
