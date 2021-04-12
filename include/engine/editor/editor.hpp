#pragma once
#include <core/ref.hpp>
#include <core/time.hpp>
#include <dumb_ecf/registry.hpp>
#include <engine/editor/log_stats.hpp>
#include <engine/editor/main_menu.hpp>
#include <engine/editor/panel.hpp>
#include <engine/editor/resizer.hpp>
#include <engine/editor/types.hpp>
#include <engine/input/state.hpp>
#include <engine/render/viewport.hpp>
#include <engine/scene/scene_node.hpp>
#include <levk_imgui/levk_imgui.hpp>

namespace le {
namespace window {
class DesktopInstance;
}

namespace edi {
struct In {
	edi::MenuList menu;
	std::vector<decf::entity_t> customEntities;
	SceneNode::Root* root = {};
	decf::registry_t* registry = {};
};
struct Out {
	struct {
		SceneNode* node = {};
		decf::entity_t entity;
	} inspecting;
};
} // namespace edi

class Editor {
  public:
	using DesktopInstance = window::DesktopInstance;

	struct Rail {
		edi::Palette panel;
		std::string_view id;
	};

	inline static Viewport s_comboView = {{0.2f, 0.0f}, {0.0f, 20.0f}, 0.6f};
	inline static bool s_engaged = false;
	inline static Rail s_left = {{}, "Left"};
	inline static Rail s_right = {{}, "Right"};

	inline static edi::In s_in;
	inline static edi::Out s_out;

	Editor();

	Viewport const& view() const noexcept;
	bool active() const noexcept;

	void update(DesktopInstance& win, input::State const& state);

  private:
	struct {
		edi::Resizer resizer;
		edi::LogStats logStats;
		edi::MainMenu menu;
		Viewport gameView = s_comboView;
		edi::In cached;
	} m_storage;
};
} // namespace le
