#pragma once
#include <string>
#include <core/version.hpp>

namespace le::os
{
Version const engineVersion();
std::string_view const gitCommitHash();
std::string_view const buildVersion();
} // namespace le::os
