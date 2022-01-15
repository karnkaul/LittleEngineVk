#include <levk/core/services.hpp>
#include <levk/engine/ecs/systems/gui_system.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/gui/view.hpp>

namespace le {
void GuiSystem::update(dens::registry const& registry) {
	if (auto eng = Services::find<Engine>()) {
		for (auto [_, c] : registry.view<gui::ViewStack>()) {
			auto& [stack] = c;
			eng->update(stack);
		}
	}
}
} // namespace le
