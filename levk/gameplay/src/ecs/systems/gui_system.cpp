#include <levk/core/services.hpp>
#include <levk/engine/engine.hpp>
#include <levk/gameplay/ecs/systems/gui_system.hpp>
#include <levk/gameplay/gui/view.hpp>

namespace le {
void GuiSystem::update(dens::registry const& registry) {
	for (auto [_, c] : registry.view<gui::ViewStack>()) {
		auto& [stack] = c;
		stack.update(data().engine.inputFrame());
	}
}
} // namespace le
