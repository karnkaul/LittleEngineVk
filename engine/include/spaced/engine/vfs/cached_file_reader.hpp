#pragma once
#include <spaced/engine/vfs/file_reader.hpp>
#include <mutex>
#include <unordered_map>

namespace spaced {
class CachedFileReader : public FileReader {
  public:
	using FileReader::FileReader;

	[[nodiscard]] auto read_bytes(Uri const& uri) -> std::vector<std::uint8_t> override;
	[[nodiscard]] auto read_string(Uri const& uri) -> std::string override;

	[[nodiscard]] auto is_loaded(Uri const& uri) const -> bool;
	[[nodiscard]] auto loaded_count() const -> std::size_t;
	auto clear_loaded() -> void;

  private:
	std::unordered_map<Uri, std::vector<std::uint8_t>, Uri::Hasher> m_cache{};
	mutable std::mutex m_mutex{};
};
} // namespace spaced
