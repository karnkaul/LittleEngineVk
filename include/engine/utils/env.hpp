#pragma once
#include <optional>
#include <clap/interpreter.hpp>
#include <core/io/path.hpp>
#include <core/std_types.hpp>

namespace le::env {
using Spec = clap::interpreter::spec_t;
using Option = clap::interpreter::option_t;
using Run = clap::interpreter::result;

Run init(int argc, char const* const argv[], Spec::cmd_map_t cmds);

///
/// \brief Obtain full path to directory containing pattern, traced from the executable path
/// \param pattern sub-path to match against
/// \param start Dir to start search from
/// \param maxHeight maximum recursive depth
///
std::optional<io::Path> findData(io::Path pattern = "data", u8 maxHeight = 10);
} // namespace le::env
