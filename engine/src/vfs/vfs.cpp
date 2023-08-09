#include <le/vfs/file_reader.hpp>
#include <le/vfs/vfs.hpp>

namespace le {
namespace {
// NOLINTNEXTLINE
std::unique_ptr<Reader> g_reader{std::make_unique<FileReader>()};
} // namespace

auto vfs::read_bytes(Uri const& uri) -> std::vector<std::byte> { return g_reader->read_bytes(uri); }
auto vfs::read_string(Uri const& uri) -> std::string { return g_reader->read_string(uri); }

auto vfs::get_reader() -> Reader& { return *g_reader; }

auto vfs::set_reader(std::unique_ptr<Reader> reader) -> void {
	if (!reader) { return; }
	g_reader = std::move(reader);
}
} // namespace le
