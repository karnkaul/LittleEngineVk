#pragma once
#include <memory>
#include <core/std_types.hpp>
#include <engine/game/input_context.hpp>

namespace le::input
{
void registerContext(std::shared_ptr<Context> context);

#if defined(LEVK_EDITOR)
void registerEditorContext(std::shared_ptr<Context> context);
#endif

glm::vec2 const& cursorPosition(bool bRaw = false);
glm::vec2 screenToWorld(glm::vec2 const& screen);
glm::vec2 worldToUI(glm::vec2 const& world);
bool isInFocus();

void setCursorMode(CursorMode mode);
CursorMode cursorMode();

} // namespace le::input
