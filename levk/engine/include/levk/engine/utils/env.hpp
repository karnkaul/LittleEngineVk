#pragma once
#include <clap/clap.hpp>
#include <levk/core/io/path.hpp>
#include <levk/core/std_types.hpp>
#include <optional>

namespace le::env {
clap::parse_result init(int argc, char const* const argv[]);

///
/// \brief Obtain full path to directory containing pattern, traced from the executable path
/// \param pattern sub-path to match against
/// \param maxHeight maximum recursive depth
///
std::optional<io::Path> findData(io::Path pattern = "data", u8 maxHeight = 10);
} // namespace le::env
