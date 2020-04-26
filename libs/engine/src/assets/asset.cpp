#include <fmt/format.h>
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/assets/asset.hpp"

namespace le::gfx
{
namespace
{
Asset::GUID g_nextGUID = 0;
}

#if defined(LEVK_ASSET_HOT_RELOAD)
Asset::File::File(stdfs::path const& id, stdfs::path const& fullPath, FileMonitor::Mode mode, std::any data)
	: monitor(fullPath, mode), id(id), data(data)
{
}
#endif

Asset::Asset(stdfs::path id) : m_id(std::move(id)), m_guid(++g_nextGUID.handle) {}

Asset::~Asset()
{
	LOG_I("-- [{}] [{}] destroyed", m_tName, m_id.generic_string());
}

void Asset::setup()
{
	m_tName = fmt::format("{} [{}]", utils::tName(*this), m_guid);
	LOG_I("== [{}] [{}] setup", m_tName, m_id.generic_string());
}

Asset::Status Asset::currentStatus() const
{
	return m_status;
}

bool Asset::isReady() const
{
	return m_status == Status::eReady;
}
} // namespace le::gfx