#pragma once
#include <memory>
#include <core/std_types.hpp>
#include <engine/game/input_context.hpp>

namespace le::input
{
using Token = std::shared_ptr<s8>;

struct CtxWrapper final
{
	Context context;
	mutable Token token;
};

void registerContext(CtxWrapper const& context);
[[nodiscard]] Token registerContext(Context const& context);

glm::vec2 const& cursorPosition(bool bIgnoreDisabled = false);
glm::vec2 screenToWorld(glm::vec2 const& screen);
glm::vec2 worldToUI(glm::vec2 const& world);
bool isInFocus();
void setActive(bool bActive);
} // namespace le::input
