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
	SharedToken(RawToken<N>&& token) : m_token(std::move(token)) {}
};

///
/// \brief Non-copiable wrapper for RawToken
///
template <typename N>
class UniqueToken final : public SharedToken<N>
{
public:
	UniqueToken() = default;
	UniqueToken(RawToken<N>&& token) : SharedToken<N>(std::move(token)) {}
	UniqueToken(UniqueToken const&) = delete;
	UniqueToken& operator=(UniqueToken const&) = delete;
	UniqueToken(UniqueToken&&) = default;
	UniqueToken& operator=(UniqueToken&&) = default;
};

///
/// \brief Wrapper for a container of T associated with RawToken<Tok_t>
///
template <typename T, typename Tok_t = s32, bool Shared = false>
struct Tokeniser final
{
	///
	/// \brief Token describes whether a T entry is valid
	///
	using Token = std::conditional_t<Shared, SharedToken<Tok_t>, UniqueToken<Tok_t>>;
	using type = typename Token::type;

	using Entry = std::pair<T, std::weak_ptr<type>>;
	std::vector<Entry> entries;

	///
	/// \brief Add an entry
	/// \returns Token that denotes entry's lifetime
	///
	[[nodiscard]] Token add(T&& t)
	{
		auto token = std::make_shared<type>(0);
		entries.push_back(std::make_pair(std::forward<T>(t), token));
		return Token(std::move(token));
	}

	///
	/// \brief Erase all invalid entries
	///
	std::size_t sweep()
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
	std::size_t clear()
	{
		auto const ret = entries.size();
		entries.clear();
	}

	///
	/// \brief Obtain Entry count
	///
	std::size_t size() const
	{
		return entries.size();
	}

	///
	/// \brief Check if no entries are present
	///
	bool empty() const
	{
		return entries.empty();
	}
};
} // namespace le
