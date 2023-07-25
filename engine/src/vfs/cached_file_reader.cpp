#include <spaced/engine/vfs/cached_file_reader.hpp>

namespace spaced {
auto CachedFileReader::read_bytes(Uri const& uri) -> std::vector<std::uint8_t> {
	auto lock = std::unique_lock{m_mutex};
	auto it = m_cache.find(uri);
	if (it == m_cache.end()) {
		lock.unlock();
		auto ret = FileReader::read_bytes(uri);
		if (ret.empty()) { return {}; }
		lock.lock();
		auto [i, _] = m_cache.insert_or_assign(uri, std::move(ret));
		it = i;
	}
	return it->second;
}

auto CachedFileReader::read_string(Uri const& uri) -> std::string { return Reader::read_string(uri); }

auto CachedFileReader::is_loaded(Uri const& uri) const -> bool {
	auto lock = std::scoped_lock{m_mutex};
	return m_cache.contains(uri);
}

auto CachedFileReader::loaded_count() const -> std::size_t {
	auto lock = std::scoped_lock{m_mutex};
	return m_cache.size();
}

auto CachedFileReader::clear_loaded() -> void {
	auto lock = std::scoped_lock{m_mutex};
	m_cache.clear();
}
} // namespace spaced
