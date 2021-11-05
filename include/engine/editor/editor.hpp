#pragma once
#include <core/time.hpp>
#include <dumb_ecf/registry.hpp>
#include <engine/editor/palette.hpp>
#include <engine/editor/types.hpp>
#include <engine/input/frame.hpp>
#include <engine/render/viewport.hpp>
#include <ktl/either.hpp>
#include <levk_imgui/levk_imgui.hpp>

#if defined(LEVK_EDITOR)
#include <engine/editor/log_stats.hpp>
#include <engine/editor/main_menu.hpp>
#include <engine/editor/resizer.hpp>
#endif

namespace le {
namespace window {
class Instance;
}
namespace graphics {
struct ScreenView;
}
namespace gui {
class TreeRoot;
}
class SceneRegistry;

namespace edi {
class Inspector;

struct In {
	static constexpr std::size_t max_custom = 16;
	using custom_t = ktl::fixed_vector<decf::entity, max_custom>;

	custom_t customEntities;
	SceneRegistry* registry = {};
};
struct Out {
	ktl::either<decf::entity, gui::TreeRoot*> inspecting;
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
	edi::Inspector& inspector() noexcept { return *m_inspector; }
	edi::Inspector const& inspector() const noexcept { return *m_inspector; }

	Rail m_left = {{}, "Left"};
	Rail m_right = {{}, "Right"};
	edi::MenuList m_menu;
	edi::In m_in;
	edi::Out m_out;

  private:
	graphics::ScreenView update(input::Frame const& frame);
	edi::Inspector* m_inspector{};

	struct {
#if defined(LEVK_EDITOR)
		edi::Resizer resizer;
		edi::LogStats logStats;
		edi::MainMenu menu;
#endif
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
