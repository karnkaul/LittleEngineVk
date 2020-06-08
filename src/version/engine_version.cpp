#include "engine/version/engine_version.hpp"
#include "version/build_version.hpp"

namespace le::os
{
Version const engineVersion()
{
	static const Version s_engineVersion(LEVK_VERSION);
	return s_engineVersion;
}

std::string_view const gitCommitHash()
{
	static const std::string_view s_gitCommitHash = LEVK_GIT_COMMIT_HASH;
	return s_gitCommitHash;
}

std::string_view const buildVersion()
{
	static const std::string_view s_buildVersion = LEVK_BUILD_VERSION;
	return s_buildVersion;
}
} // namespace le::os
