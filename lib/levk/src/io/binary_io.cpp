#include <levk/io/binary_io.hpp>
#include <cstring>

namespace levk::binary {
auto Header::read_from(std::span<std::byte const>& bytes) -> bool {
	static constexpr auto size_v = sizeof(Header);
	if (bytes.size() < size_v) { return false; }
	auto header = Header{};
	std::memcpy(&header, bytes.data(), size_v);
	if (header.version != version_v) { return false; }
	bytes = bytes.subspan(size_v);
	*this = header;
	return true;
}

Reader::Reader(std::span<std::byte const> bytes) : m_bytes(bytes) { next(); }

void Reader::next() {
	m_current.reset();

	if (m_bytes.empty()) { return; }

	auto header = Header{};
	if (!header.read_from(m_bytes)) {
		m_bytes = {};
		return;
	}

	m_current = header;
}
} // namespace levk::binary
