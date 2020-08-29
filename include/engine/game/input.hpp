#pragma once
#include <core/std_types.hpp>
#include <engine/game/input_context.hpp>

namespace le::input
{
[[nodiscard]] Token registerContext(Context const* pContext);

glm::vec2 const& cursorPosition(bool bRaw = false);
glm::vec2 screenToWorld(glm::vec2 const& screen);
glm::vec2 worldToUI(glm::vec2 const& world);
bool focused();

void setCursorMode(CursorMode mode);
CursorMode cursorMode();

} // namespace le::input
