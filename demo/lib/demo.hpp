#pragma once
#include <core/erased_ref.hpp>
#include <core/io/reader.hpp>

namespace le::demo {
struct CreateInfo {
	struct Args {
		int argc = 0;
		char const* const* argv = nullptr;
	};

	Args args;
	ErasedRef androidApp;
};

bool run(CreateInfo const& info, io::Reader const& reader);
} // namespace le::demo
