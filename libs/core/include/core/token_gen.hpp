#pragma once
#include <algorithm>
#include <deque>
#include <core/counter.hpp>
#include <core/std_types.hpp>
#include <core/zero.hpp>

namespace le
{
///
/// \brief RAII Token (moveable) that will remove its corresponding entry on destruction
///
struct Token;

namespace detail
{
struct Base
{
	using ID = TZero<u64>;
	using id_type = ID::type;
	static constexpr id_type null = ID::null;

	static constexpr ID increment(ID& out_id, bool bPost);

	virtual void pop(ID id) = 0;
};
} // namespace detail

///
/// \brief Wrapper for a container of T associated with RAII tokens
///
/// Important: generator instance must outlive all Token instances handed out by it
///
template <typename T, template <typename...> typename C = std::deque, typename... Args>
class TTokenGen final : detail::Base
{
public:
	///
	/// \brief Add a new entry
	/// \returns Token (moveable) that will remove entry on destruction
	///
	template <bool Front = false>
	Token push(T t);
	///
	/// \brief Clear all entries
	///
	std::size_t clear() noexcept;
	///
	/// \brief Obtain Entry count
	///
	std::size_t size() const noexcept;
	///
	/// \brief Check if no entries are present
	///
	bool empty() const noexcept;
	///
	/// \brief Iterate over all present entries
	///
	template <typename F>
	void forEach(F f);
	///
	/// \brief Iterate over all present entries
	///
	template <typename F>
	void forEach(F f) const;

private:
	using ID = detail::Base::ID;
	using Container = C<std::pair<T, ID>, Args...>;
	using id_type = detail::Base::id_type;
	static constexpr id_type null = detail::Base::null;

private:
	Container m_entries;
	ID m_nextID = 1;

private:
	void pop(ID id) override;

private:
	friend struct Token;
};

struct Token : NoCopy
{
private:
	using ID = detail::Base::ID;
	using id_type = detail::Base::id_type;
	static constexpr id_type null = detail::Base::null;

public:
	constexpr Token(detail::Base* pParent = nullptr, ID id = null) noexcept;
	constexpr Token(Token&& rhs) noexcept;
	constexpr Token& operator=(Token&& rhs) noexcept;
	~Token();

	///
	/// \brief Check if this instance is valid
	///
	constexpr bool valid() const;

private:
	detail::Base* pParent;
	ID id;
};

inline constexpr detail::Base::ID detail::Base::increment(ID& out_id, bool bPost)
{
	return bPost ? out_id.payload++ : ++out_id.payload;
}

template <typename T, template <typename...> typename C, typename... Args>
template <bool Front>
Token TTokenGen<T, C, Args...>::push(T t)
{
	Token ret(this, m_nextID);
	if constexpr (Front)
	{
		m_entries.push_front(std::make_pair(std::move(t), detail::Base::increment(m_nextID, true)));
	}
	else
	{
		m_entries.push_back(std::make_pair(std::move(t), detail::Base::increment(m_nextID, true)));
	}
	return ret;
}

template <typename T, template <typename...> typename C, typename... Args>
std::size_t TTokenGen<T, C, Args...>::clear() noexcept
{
	auto const ret = m_entries.size();
	m_entries.clear();
	return ret;
}

template <typename T, template <typename...> typename C, typename... Args>
std::size_t TTokenGen<T, C, Args...>::size() const noexcept
{
	return m_entries.size();
}

template <typename T, template <typename...> typename C, typename... Args>
bool TTokenGen<T, C, Args...>::empty() const noexcept
{
	return m_entries.empty();
}

template <typename T, template <typename...> typename C, typename... Args>
template <typename F>
void TTokenGen<T, C, Args...>::forEach(F f)
{
	for (auto& [t, _] : m_entries)
	{
		f(t);
	}
}

template <typename T, template <typename...> typename C, typename... Args>
template <typename F>
void TTokenGen<T, C, Args...>::forEach(F f) const
{
	for (auto& [t, _] : m_entries)
	{
		f(t);
	}
}

template <typename T, template <typename...> typename C, typename... Args>
void TTokenGen<T, C, Args...>::pop(ID id)
{
	auto search = std::find_if(m_entries.begin(), m_entries.end(), [id](auto const& entry) -> bool { return entry.second == id; });
	if (search != m_entries.end())
	{
		m_entries.erase(search);
	}
}

inline constexpr Token::Token(detail::Base* pParent, ID id) noexcept : pParent(pParent), id(id) {}

inline constexpr Token::Token(Token&& rhs) noexcept : pParent(rhs.pParent), id(rhs.id)
{
	rhs.pParent = nullptr;
	rhs.id = null;
}

inline constexpr Token& Token::operator=(Token&& rhs) noexcept
{
	if (&rhs != this)
	{
		if (valid())
		{
			pParent->pop(id);
		}
		pParent = rhs.pParent;
		id = rhs.id;
		rhs.pParent = nullptr;
		rhs.id = null;
	}
	return *this;
}

inline Token::~Token()
{
	if (valid())
	{
		pParent->pop(id);
	}
}

inline constexpr bool Token::valid() const
{
	return id > 0 && pParent != nullptr;
}
} // namespace le
