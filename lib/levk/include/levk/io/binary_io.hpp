#pragma once
#include <levk/core/enum_array.hpp>
#include <levk/core/not_null.hpp>
#include <levk/core/ptr.hpp>
#include <array>
#include <cstddef>
#include <cstring>
#include <optional>

namespace levk::binary {
/// \brief Concept for serializable data.
template <typename Type>
concept BinaryDataT = std::is_trivially_destructible_v<Type>;

/// \brief Header metadata.
enum class Metadata : int { eType, eData, eCOUNT_ };

/// \brief Current Header version.
inline constexpr auto version_v = std::int32_t{2};

/// \brief Customizable payload per Header.
using Payload = std::array<std::byte, 32>;

/// \brief Header for serialized data.
struct Header {
	std::int32_t version{};
	Payload payload{};
	EnumArray<Metadata, std::size_t> sizes{};

	template <BinaryDataT Type>
	[[nodiscard]] static constexpr auto build(std::span<Type const> bytes, Payload payload = {}) -> Header {
		auto ret = Header{.version = version_v, .payload = payload};
		ret.sizes[Metadata::eType] = sizeof(Type);
		ret.sizes[Metadata::eData] = bytes.size_bytes();
		return ret;
	}

	[[nodiscard]] auto read_from(std::span<std::byte const>& bytes) -> bool;

	[[nodiscard]] constexpr auto get_count() const -> std::size_t {
		if (sizes[Metadata::eType] == 0) { return 0; }
		return sizes[Metadata::eData] / sizes[Metadata::eType];
	}
};

static_assert(BinaryDataT<Header>);

/// \brief Binary reader.
class Reader {
  public:
	explicit Reader(std::span<std::byte const> bytes);

	[[nodiscard]] auto get_current_header() const -> Ptr<Header const> { return m_current ? &*m_current : nullptr; }

	template <BinaryDataT Type>
	[[nodiscard]] auto read_next(std::vector<Type>& out) -> bool {
		if (!m_current || m_current->sizes[Metadata::eType] != sizeof(Type)) { return false; }
		if (m_current->get_count() == 0) { return true; }
		auto const size_bytes = m_current->sizes[Metadata::eData];
		auto const offset = out.size();
		out.resize(out.size() + m_current->get_count());
		auto* start = out.data() + offset; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		std::memcpy(start, m_bytes.data(), size_bytes);
		m_bytes = m_bytes.subspan(size_bytes);
		next();
		return true;
	}

  private:
	void next();

	std::span<std::byte const> m_bytes{};
	std::optional<Header> m_current{};
};

/// \brief Binary writer.
class Writer {
  public:
	explicit Writer(std::vector<std::byte>& out) : m_buffer(&out) {}

	template <BinaryDataT Type>
	void write_next(std::span<Type> values, Payload payload = {}) {
		auto const header = Header::build(values, payload);
		auto const offset = m_buffer->size();
		m_buffer->resize(m_buffer->size() + sizeof(Header) + values.size_bytes());
		auto* start = m_buffer->data() + offset; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		std::memcpy(start, &header, sizeof(Header));
		start += sizeof(Header); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		std::memcpy(start, values.data(), values.size_bytes());
	}

  private:
	NotNull<std::vector<std::byte>*> m_buffer;
};
} // namespace levk::binary
