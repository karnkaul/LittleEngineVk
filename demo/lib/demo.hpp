#pragma once
#include <core/erased_ref.hpp>
#include <core/io/reader.hpp>

namespace le::demo {
bool run(io::Reader const& reader, ErasedRef androidApp = {});
} // namespace le::demo
