#pragma once
#include <core/io/path.hpp>
#include <core/std_types.hpp>
#include <optional>

#include <clap/clap.hpp>

namespace le::env {
clap::parse_result init(int argc, char const* const argv[]);

///
/// \brief Obtain full path to directory containing pattern, traced from the executable path
/// \param pattern sub-path to match against
/// \param start Dir to start search from
/// \param maxHeight maximum recursive depth
///
std::optional<io::Path> findData(io::Path pattern = "data", u8 maxHeight = 10);
} // namespace le::env
