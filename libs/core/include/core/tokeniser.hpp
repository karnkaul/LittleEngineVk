#pragma once
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>
#include <core/std_types.hpp>

namespace le
{
template <typename N>
using RawToken = std::shared_ptr<N>;

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

template <typename T, typename N = s32, bool Shared = false>
struct Tokeniser final
{
	using Token = std::conditional_t<Shared, SharedToken<N>, UniqueToken<N>>;
	using type = typename Token::type;

	using Entry = std::pair<T, std::weak_ptr<type>>;
	std::vector<Entry> entries;

	[[nodiscard]] Token add(T&& t)
	{
		auto token = std::make_shared<type>(0);
		entries.push_back(std::make_pair(std::forward<T>(t), token));
		return Token(std::move(token));
	}

	std::size_t sweep()
	{
		auto const before = entries.size();
		auto iter =
			std::remove_if(std::begin(entries), std::end(entries), [](auto const& entry) -> bool { return !std::get<std::weak_ptr<type>>(entry).lock(); });
		entries.erase(iter, entries.end());
		return before - entries.size();
	}

	std::size_t clear()
	{
		auto const ret = entries.size();
		entries.clear();
	}

	std::size_t size() const
	{
		return entries.size();
	}

	bool empty() const
	{
		return entries.empty();
	}
};
} // namespace le
