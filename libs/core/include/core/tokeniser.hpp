#pragma once
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Typedef for core token type
///
template <typename N>
using RawToken = std::shared_ptr<N>;

///
/// \brief Copiable wrapper for RawToken
///
template <typename N>
class SharedToken
{
public:
	using type = N;

protected:
	RawToken<N> m_token;

public:
	SharedToken() = default;
	explicit SharedToken(RawToken<N>&& token) noexcept : m_token(std::move(token)) {}
};

///
/// \brief Non-copiable wrapper for RawToken
///
template <typename N>
class UniqueToken final : public SharedToken<N>
{
public:
	UniqueToken() = default;
	explicit UniqueToken(RawToken<N>&& token) noexcept : SharedToken<N>(std::move(token)) {}
	UniqueToken(UniqueToken const&) = delete;
	UniqueToken& operator=(UniqueToken const&) = delete;
	UniqueToken(UniqueToken&&) noexcept = default;
	UniqueToken& operator=(UniqueToken&&) noexcept = default;
};

///
/// \brief Wrapper for a container of T associated with RawToken<Tok_t>
///
template <typename T, typename Tok_t = s32>
struct Tokeniser final
{
	using type = Tok_t;

	using Entry = std::pair<T, std::weak_ptr<type>>;
	std::vector<Entry> entries;

	///
	/// \brief Add an entry
	/// \returns SharedToken that denotes entry's lifetime
	///
	[[nodiscard]] SharedToken<type> addShared(T&& t)
	{
		auto token = std::make_shared<type>(0);
		entries.push_back(std::make_pair(std::forward<T>(t), token));
		return SharedToken<type>(std::move(token));
	}

	///
	/// \brief Add an entry
	/// \returns UniqueToken that denotes entry's lifetime
	///
	[[nodiscard]] UniqueToken<type> addUnique(T&& t)
	{
		auto token = std::make_shared<type>(0);
		entries.push_back(std::make_pair(std::forward<T>(t), token));
		return UniqueToken<type>(std::move(token));
	}

	///
	/// \brief Erase all invalid entries
	///
	std::size_t sweep() noexcept
	{
		auto const before = entries.size();
		auto iter =
			std::remove_if(std::begin(entries), std::end(entries), [](auto const& entry) -> bool { return !std::get<std::weak_ptr<type>>(entry).lock(); });
		entries.erase(iter, entries.end());
		return before - entries.size();
	}

	///
	/// \brief Clear all entries
	/// \returns Entry count before clearing
	///
	std::size_t clear() noexcept
	{
		auto const ret = entries.size();
		entries.clear();
	}

	///
	/// \brief Obtain Entry count
	///
	std::size_t size() const noexcept
	{
		return entries.size();
	}

	///
	/// \brief Check if no entries are present
	///
	bool empty() const noexcept
	{
		return entries.empty();
	}
};
} // namespace le
