#include "engine_version.hpp"
#include "build_version.hpp"

namespace le::os
{
Version const engineVersion()
{
	static const Version s_engineVersion(LE3D_VERSION);
	return s_engineVersion;
}

std::string_view const gitCommitHash()
{
	static const std::string_view s_gitCommitHash = LE3D_GIT_COMMIT_HASH;
	return s_gitCommitHash;
}

std::string_view const buildVersion()
{
	static const std::string_view s_buildVersion = LE3D_BUILD_VERSION;
	return s_buildVersion;
}
} // namespace le::os
