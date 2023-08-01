#pragma once
#include <string>

namespace spaced {
class Uri {
  public:
	struct Hasher;

	Uri() = default;

	Uri(std::string value) : m_value(std::move(value)), m_hash(std::hash<std::string>{}(m_value)) {}
	Uri(char const* value) : Uri(std::string{value}) {}

	[[nodiscard]] auto value() const -> std::string_view { return m_value; }
	[[nodiscard]] auto c_str() const -> char const* { return m_value.c_str(); }

	[[nodiscard]] auto hash() const -> std::size_t { return m_hash; }
	[[nodiscard]] auto extension() const -> std::string;

	[[nodiscard]] auto absolute(std::string_view prefix) const -> Uri;

	[[nodiscard]] auto is_empty() const -> bool { return m_value.empty(); }

	operator std::string_view() const { return value(); }
	explicit operator bool() const { return !is_empty(); }

	auto operator==(Uri const& uri) const -> bool { return m_value == uri.m_value; }

  private:
	std::string m_value{};
	std::size_t m_hash{};
};

struct Uri::Hasher {
	using is_transparent = void;

	auto operator()(Uri const& uri) const { return uri.hash(); }

	template <std::constructible_from<std::string_view> T>
	auto operator()(T const& uri) const {
		return std::hash<std::string_view>{}(std::string_view{uri});
	}
};
} // namespace spaced
