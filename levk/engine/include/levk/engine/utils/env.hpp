#pragma once
#include <clap/clap.hpp>
#include <levk/core/io/path.hpp>
#include <levk/core/std_types.hpp>
#include <optional>

namespace le::env {
enum class ExitCode { eSuccess = 0, eFailure = 10 };

constexpr int exitCode(bool result) { return result ? static_cast<int>(ExitCode::eSuccess) : static_cast<int>(ExitCode::eFailure); }

clap::parse_result init(int argc, char const* const argv[]);

///
/// \brief Obtain full path to directory containing pattern, traced from the executable path
/// \param pattern sub-path to match against
///
io::Path findData(io::Path pattern = "data");
} // namespace le::env
