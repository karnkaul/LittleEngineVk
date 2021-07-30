#pragma once
#include <core/time.hpp>
#include <dumb_ecf/registry.hpp>
#include <engine/editor/log_stats.hpp>
#include <engine/editor/main_menu.hpp>
#include <engine/editor/panel.hpp>
#include <engine/editor/resizer.hpp>
#include <engine/editor/types.hpp>
#include <engine/input/frame.hpp>
#include <engine/render/viewport.hpp>
#include <levk_imgui/levk_imgui.hpp>

namespace le {
namespace window {
class DesktopInstance;
}
namespace graphics {
struct ScreenView;
}
class SceneRegistry;

namespace edi {
struct In {
	static constexpr std::size_t max_custom = 16;
	using custom_t = ktl::fixed_vector<decf::entity, max_custom>;

	custom_t customEntities;
	SceneRegistry* registry = {};
};
struct Out {
	struct {
		SceneNode* node = {};
		decf::entity entity;
	} inspecting;
};
} // namespace edi

class Editor {
  public:
	struct Rail {
		edi::Palette panel;
		std::string_view id;
	};

	inline static Viewport s_comboView = {{0.2f, 0.0f}, {0.0f, 20.0f}, 0.6f};

	Editor();

	void bindNextFrame(not_null<SceneRegistry*> registry, edi::In::custom_t const& custom = {});
	bool engaged() const noexcept { return m_storage.engaged; }
	void engage(bool set) noexcept { m_storage.engaged = set; }
	void toggle() noexcept { engage(!engaged()); }

	Viewport const& view() const noexcept;
	bool active() const noexcept;

	bool draw(graphics::CommandBuffer cb) const;

	Rail m_left = {{}, "Left"};
	Rail m_right = {{}, "Right"};
	edi::MenuList m_menu;
	edi::In m_in;
	edi::Out m_out;

  private:
	graphics::ScreenView update(input::Frame const& frame);

	struct {
		edi::Resizer resizer;
		edi::LogStats logStats;
		edi::MainMenu menu;
		Viewport gameView = s_comboView;
		edi::In cached;
		bool engaged{};
	} m_storage;

	friend class Engine;
};

inline void Editor::bindNextFrame(not_null<SceneRegistry*> registry, edi::In::custom_t const& custom) {
	m_in.registry = registry;
	m_in.customEntities = custom;
}
} // namespace le
