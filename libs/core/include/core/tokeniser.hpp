#pragma once
#include <algorithm>
#include <deque>
#include <core/std_types.hpp>

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
	virtual void pop(u64 id) = 0;
};
} // namespace detail

///
/// \brief Wrapper for a container of Ts associated with RAII tokens
///
/// Important: Tokeniser instance must outlive all Token instances handed out by it
///
template <typename T, template <typename...> typename C = std::deque, typename... Args>
class Tokeniser final : detail::Base
{
public:
	using Container = C<std::pair<T, u64>, Args...>;

public:
	///
	/// \brief Add a new entry
	/// \returns Token (moveable) that will remove entry on destruction
	///
	Token pushBack(T t);
	///
	/// \brief Add a new entry
	/// \returns Token (moveable) that will remove entry on destruction
	///
	Token pushFront(T t);
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
	Container m_entries;
	u64 m_nextID = 1;

private:
	void pop(u64 id) override;

private:
	friend struct Token;
};

struct Token : NoCopy
{
public:
	constexpr Token(detail::Base* pParent = nullptr, u64 id = 0) noexcept : pParent(pParent), id(id) {}

	constexpr Token(Token&& rhs) noexcept : pParent(rhs.pParent), id(rhs.id)
	{
		rhs.pParent = nullptr;
		rhs.id = 0;
	}

	constexpr Token& operator=(Token&& rhs) noexcept
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
			rhs.id = 0;
		}
		return *this;
	}

	~Token()
	{
		if (valid())
		{
			pParent->pop(id);
		}
	}

	constexpr bool valid() const
	{
		return id > 0 && pParent != nullptr;
	}

private:
	detail::Base* pParent;
	u64 id;
};

template <typename T, template <typename...> typename C, typename... Args>
Token Tokeniser<T, C, Args...>::pushBack(T t)
{
	Token ret(this, m_nextID);
	m_entries.push_back(std::make_pair(std::move(t), m_nextID++));
	return ret;
}

template <typename T, template <typename...> typename C, typename... Args>
Token Tokeniser<T, C, Args...>::pushFront(T t)
{
	Token ret(this, m_nextID);
	m_entries.push_front(std::make_pair(std::move(t), m_nextID++));
	return ret;
}

template <typename T, template <typename...> typename C, typename... Args>
std::size_t Tokeniser<T, C, Args...>::clear() noexcept
{
	auto const ret = m_entries.size();
	m_entries.clear();
	return ret;
}

template <typename T, template <typename...> typename C, typename... Args>
std::size_t Tokeniser<T, C, Args...>::size() const noexcept
{
	return m_entries.size();
}

template <typename T, template <typename...> typename C, typename... Args>
bool Tokeniser<T, C, Args...>::empty() const noexcept
{
	return m_entries.empty();
}

template <typename T, template <typename...> typename C, typename... Args>
template <typename F>
void Tokeniser<T, C, Args...>::forEach(F f)
{
	for (auto& [t, _] : m_entries)
	{
		f(t);
	}
}

template <typename T, template <typename...> typename C, typename... Args>
template <typename F>
void Tokeniser<T, C, Args...>::forEach(F f) const
{
	for (auto& [t, _] : m_entries)
	{
		f(t);
	}
}

template <typename T, template <typename...> typename C, typename... Args>
void Tokeniser<T, C, Args...>::pop(u64 id)
{
	auto search = std::find_if(m_entries.begin(), m_entries.end(), [id](auto const& entry) -> bool { return entry.second == id; });
	if (search != m_entries.end())
	{
		m_entries.erase(search);
	}
}
} // namespace le
