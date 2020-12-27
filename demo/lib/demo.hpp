#pragma once
#include <core/erased_ref.hpp>

namespace le::demo {
struct CreateInfo {
	struct Args {
		int argc = 0;
		char const* const* argv = nullptr;
	};

	Args args;
	ErasedRef androidApp;
};

bool run(CreateInfo const& info);
} // namespace le::demo
