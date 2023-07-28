#include <spaced/vfs/uri.hpp>
#include <filesystem>

namespace spaced {
namespace fs = std::filesystem;

auto Uri::extension() const -> std::string { return fs::path{m_value}.extension().string(); }

auto Uri::absolute(std::string_view const prefix) const -> Uri { return (fs::path{prefix} / value()).generic_string(); }
} // namespace spaced
