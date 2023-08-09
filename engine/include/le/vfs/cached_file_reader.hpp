#pragma once
#include <le/vfs/file_reader.hpp>
#include <mutex>
#include <unordered_map>

namespace le {
class CachedFileReader : public FileReader {
  public:
	using FileReader::FileReader;

	[[nodiscard]] auto read_bytes(Uri const& uri) -> std::vector<std::byte> override;
	[[nodiscard]] auto read_string(Uri const& uri) -> std::string override;

	[[nodiscard]] auto is_loaded(Uri const& uri) const -> bool;
	[[nodiscard]] auto loaded_count() const -> std::size_t;
	auto clear_loaded() -> void;

  private:
	std::unordered_map<Uri, std::vector<std::byte>, Uri::Hasher> m_cache{};
	mutable std::mutex m_mutex{};
};
} // namespace le
