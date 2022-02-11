#pragma once
#include <cstdint>

namespace le::utils {
class TypeGUID {
  public:
	using type = std::size_t;

	template <typename T>
	static TypeGUID make() noexcept {
		static TypeGUID const ret = {s_next++};
		return ret;
	}

	constexpr TypeGUID() = default;

	constexpr bool operator==(TypeGUID const&) const = default;
	constexpr operator type() const noexcept { return value(); }
	constexpr type value() const noexcept { return m_hash; }

  private:
	constexpr TypeGUID(type hash) noexcept : m_hash(hash) {}

	inline static type s_next{};
	type m_hash{};
};
} // namespace le::utils
