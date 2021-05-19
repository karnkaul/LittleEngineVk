#pragma once
#include <core/erased_ptr.hpp>
#include <core/io/reader.hpp>

namespace le::demo {
bool run(io::Reader const& reader, ErasedPtr androidApp = {});
} // namespace le::demo
