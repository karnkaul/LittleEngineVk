#pragma once
#include <array>
#include <cstring>
#include <format>

namespace spaced {
///
/// \brief Simple "stringy" wrapper around a stack buffer of chars.
///
/// Stores a buffer of Capacity + 1 to guarantee null termination.
/// Note: chars in input strings beyond Capacity are silently dropped.
///
template <std::size_t Capacity = 64>
class FixedString {
  public:
	static constexpr auto capacity_v{Capacity};

	FixedString() = default;

	///
	/// \brief Construct an instance by copying t into the internal buffer.
	/// \param t Direct string to copy
	///
	template <std::convertible_to<std::string_view> T>
	FixedString(T const& t) {
		auto const str = std::string_view{t};
		m_size = str.size();
		std::memcpy(m_buffer.data(), str.data(), m_size);
	}

	///
	/// \brief Construct an instance by formatting into the internal buffer.
	/// \param fmt Format string
	/// \param args... Format arguments
	///
	template <typename... Args>
	explicit FixedString(std::format_string<Args...> fmt, Args&&... args) {
		auto const [it, _] = std::format_to_n(m_buffer.data(), Capacity, fmt, std::forward<Args>(args)...);
		// NOLINTNEXTLINE
		m_size = static_cast<std::size_t>(std::distance(m_buffer.data(), it));
	}

	///
	/// \brief Append a string to the internal buffer.
	/// \param rhs The string to append.
	///
	template <std::size_t N>
	void append(FixedString<N> const& rhs) {
		auto const dsize = std::min(Capacity - m_size, rhs.size());
		std::memcpy(m_buffer.data() + m_size, rhs.data(), dsize);
		m_size += dsize;
	}

	///
	/// \brief Obtain a view into the stored string.
	/// \returns A view into the stored string
	///
	[[nodiscard]] auto view() const -> std::string_view { return {m_buffer, m_size}; }
	///
	/// \brief Obtain a pointer to the start of the stored string.
	/// \returns A const pointer to the start of the string
	///
	[[nodiscard]] auto data() const -> char const* { return m_buffer; }
	///
	/// \brief Obtain a pointer to the start of the stored string.
	/// \returns A const pointer to the start of the string
	///
	[[nodiscard]] auto c_str() const -> char const* { return m_buffer.data(); }
	///
	/// \brief Obtain the size of the stored string.
	/// \returns The number of characters in the string
	///
	[[nodiscard]] auto size() const -> std::size_t { return m_size; }
	///
	/// \brief Check if the stored string is empty.
	/// \returns true If the size is 0
	///
	[[nodiscard]] auto empty() const -> bool { return m_size == 0; }

	operator std::string_view() const { return view(); }

	template <std::size_t N>
	auto operator+=(FixedString<N> const& rhs) -> FixedString& {
		append(rhs);
		return *this;
	}

	auto operator==(FixedString const& rhs) const -> bool { return view() == rhs.view(); }

  private:
	std::array<char, Capacity + 1> m_buffer{};
	std::size_t m_size{};
};
} // namespace spaced
