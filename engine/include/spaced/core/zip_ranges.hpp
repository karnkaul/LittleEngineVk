#pragma once
#include <algorithm>
#include <span>
#include <tuple>

namespace spaced {
///
/// \brief Wrapper over a tuple of iterators.
///
template <typename... It>
class ZipIterator {
  public:
	using value_type = std::tuple<decltype(*std::declval<It>())...>;
	using difference_type = std::ptrdiff_t;

	///
	/// \brief Pack of iterators stored as a tuple.
	///
	/// Used to enable passing of multiple packs in a single function / constructor.
	///
	struct IterPack {
		std::tuple<It...> its;
		constexpr IterPack(It... its) : its{its...} {}
	};

	///
	/// \brief Pseudo-container to "march" along one or more iterator packs.
	///
	class Container {
	  public:
		using iterator = ZipIterator<It...>;
		using const_iterator = iterator;

		constexpr Container(IterPack first, IterPack last)
			: m_first(std::move(first)), m_last(std::move(last)), m_size(min_distance(std::index_sequence_for<It...>())) {}

		constexpr auto begin() const { return iterator{m_first.its, 0u}; }
		constexpr auto end() const { return iterator{m_last.its, m_size}; }

	  private:
		template <std::size_t... I>
		constexpr auto min_distance(std::index_sequence<I...>) const -> std::size_t {
			return static_cast<std::size_t>(std::min({std::distance(std::get<I>(m_first.its), std::get<I>(m_last.its))...}));
		}

		IterPack m_first;
		IterPack m_last;
		std::size_t m_size;
	};

	ZipIterator() = default;

	constexpr auto operator*() const -> value_type { return unpack(std::index_sequence_for<It...>()); }
	constexpr auto operator++() -> ZipIterator& { return increment(std::index_sequence_for<It...>()); }
	constexpr auto operator++(int) -> ZipIterator {
		auto ret = *this;
		increment(std::index_sequence_for<It...>());
		return ret;
	}

	template <std::size_t... I>
	constexpr auto unpack(std::index_sequence<I...>) const {
		return value_type{*std::get<I>(m_it)...};
	}

	template <std::size_t... I>
	constexpr auto increment(std::index_sequence<I...>) -> ZipIterator& {
		(++std::get<I>(m_it), ...);
		++m_index;
		return *this;
	}

	[[nodiscard]] constexpr auto index() const -> std::size_t { return m_index; }

	constexpr auto operator==(ZipIterator const& rhs) const -> bool { return m_index == rhs.m_index; }

  private:
	constexpr ZipIterator(std::tuple<It...> it, std::size_t index) : m_it(std::move(it)), m_index(index) {}

	std::tuple<It...> m_it{};
	std::size_t m_index{};
};

void zip_ranges() = delete;

///
/// \brief Iterate in lock-step over one or more containers.
///
/// Usage: for (auto [a, b, c] : zip(x, y, z)) { /* ... */ }
///
template <typename... Containers>
constexpr auto zip_ranges(Containers&&... containers) {
	using Ret = typename ZipIterator<decltype(std::begin(containers))...>::Container;
	return Ret{{std::begin(containers)...}, {std::end(containers)...}};
}
} // namespace spaced
