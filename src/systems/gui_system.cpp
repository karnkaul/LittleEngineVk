#include <core/services.hpp>
#include <engine/engine.hpp>
#include <engine/gui/view.hpp>
#include <engine/systems/gui_system.hpp>

namespace le {
void GuiSystem::tick(dens::registry const& registry, Time_s) {
	if (auto eng = Services::find<Engine>()) {
		for (auto [_, c] : registry.view<gui::ViewStack>()) {
			auto& [stack] = c;
			eng->update(stack);
		}
	}
}
} // namespace le