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
Token registerContext(Context const& context);

glm::vec2 const& cursorPosition();
} // namespace le::input
