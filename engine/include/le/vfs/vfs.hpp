#pragma once
#include <le/vfs/reader.hpp>
#include <memory>
#include <span>

namespace le::vfs {
[[nodiscard]] auto read_bytes(Uri const& uri) -> std::vector<std::uint8_t>;
[[nodiscard]] auto read_string(Uri const& uri) -> std::string;

[[nodiscard]] auto get_reader() -> Reader&;
auto set_reader(std::unique_ptr<Reader> reader) -> void;
} // namespace le::vfs
