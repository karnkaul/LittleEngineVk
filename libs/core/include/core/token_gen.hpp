#pragma once
#include <algorithm>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>
#include <core/std_types.hpp>

namespace le {
namespace detail {
struct TokenGenBase;
}

///
/// \brief RAII Token (moveable) that will remove its corresponding entry on destruction
///
struct Token : NoCopy {
	using ID = u64;
	using id_type = ID;
	static constexpr ID null = 0;

	constexpr Token(detail::TokenGenBase* pParent = nullptr, ID id = null) noexcept;
	constexpr Token(Token&& rhs) noexcept;
	Token& operator=(Token&& rhs) noexcept;
	~Token();

	///
	/// \brief Check if this instance is valid
	///
	constexpr bool valid() const;

  private:
	detail::TokenGenBase* pParent;
	ID id;

	template <typename T, typename TGS>
	friend class TTokenGen;
};

///
/// \brief Base type for tagging TTokenGen storage category
///
struct TGS_Tag {};
///
/// \brief Tag for sequential containers
///
struct TGS_Tag_Seq : TGS_Tag {};
///
/// \brief Tag for associative containers
///
struct TGS_Tag_Ass : TGS_Tag {};

namespace detail {
template <typename Tag, typename T>
struct TGStorage_Base {
	using type = T;
	using tag = Tag;
};
} // namespace detail

///
/// \brief Base type for tagged storage
///
template <typename Tag, typename T, template <typename...> typename C, typename... Args>
struct TGStorage {
	static_assert(std::is_base_of_v<TGS_Tag, Tag>, "Invalid Tag");
};
///
/// \brief Specialised type for TGS_Tag_Seq
///
template <typename T, template <typename...> typename C, typename... Args>
struct TGStorage<TGS_Tag_Seq, T, C, Args...> : detail::TGStorage_Base<TGS_Tag_Seq, T> {
	using container_type = C<std::pair<Token::id_type, T>, Args...>;
};
///
/// \brief Specialised type for TGS_Tag_Ass
///
template <typename T, template <typename...> typename C, typename... Args>
struct TGStorage<TGS_Tag_Ass, T, C, Args...> : detail::TGStorage_Base<TGS_Tag_Ass, T> {
	using container_type = C<Token::id_type, T, Args...>;
};
///
/// \brief Base type for TTokenGen storage specification
///
struct TGSpec {};
///
/// \brief TGSpec for std::vector<std::pair<Token::ID, T>, Args...>
///
struct TGSpec_vector : TGSpec {
	template <typename T>
	using storage = TGStorage<TGS_Tag_Seq, T, std::vector>;
};
///
/// \brief TGSpec for std::deque<std::pair<Token::ID, T>, Args...>
///
struct TGSpec_deque : TGSpec {
	template <typename T>
	using storage = TGStorage<TGS_Tag_Seq, T, std::deque>;
};
///
/// \brief TGSpec for std::unordered_map<Token::ID, T, Args...>
///
struct TGSpec_umap : TGSpec {
	template <typename T>
	using storage = TGStorage<TGS_Tag_Ass, T, std::unordered_map>;
};

namespace detail {
struct TokenGenBase {
	static constexpr Token::ID increment(Token::ID);

  protected:
	virtual void pop(Token::id_type id) = 0;

	friend struct le::Token;
};
} // namespace detail

///
/// \brief Wrapper for a container of T associated with RAII tokens
///
/// Important: generator instance must outlive all Token instances handed out by it
///
template <typename T, typename Spec = TGSpec_vector>
class TTokenGen final : detail::TokenGenBase {
	static_assert(std::is_base_of_v<TGSpec, Spec>, "Invalid Spec");

  public:
	using type = T;
	using storage = typename Spec::template storage<T>;
	using tag = typename storage::tag;
	using container_type = typename storage::container_type;

	struct iterator;
	struct const_iterator;

	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;

	///
	/// \brief Add a new entry
	/// \returns Token (moveable) that will remove entry on destruction
	///
	template <bool Front = false, typename U>
	Token push(U&& u);
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
	/// \brief Search for a T mapped to a token
	///
	T const* find(Token const& token) const;
	///
	/// \brief Search for a T mapped to a token
	///
	T* find(Token const& token);

  private:
	container_type m_entries;
	Token::ID m_nextID = Token::null;

	void pop(Token::id_type id) override;
};

// impl

namespace detail {
template <bool Front, typename TGS, typename T>
void tgsPush(typename TGS::container_type& out_tgs, Token::id_type id, T&& t) {
	if constexpr (std::is_base_of_v<TGS_Tag_Seq, typename TGS::tag>) {
		if constexpr (Front) {
			out_tgs.emplace_front(id, std::forward<T>(t));
		} else {
			out_tgs.emplace_back(id, std::forward<T>(t));
		}
	} else {
		static_assert(!Front, "Cannot use Front on associative containers");
		out_tgs.emplace(id, std::forward<T>(t));
	}
}
template <typename Tag, typename T, typename TGSC>
T* tgsFind(TGSC&& container, Token::id_type id) {
	if constexpr (std::is_base_of_v<TGS_Tag_Seq, Tag>) {
		auto search = std::find_if(container.begin(), container.end(), [id](auto const& entry) -> bool { return entry.first == id; });
		if (search != container.end()) {
			return &search->second;
		}
	} else {
		if (auto search = container.find(id); search != container.end()) {
			return &search->second;
		}
	}
	return nullptr;
}
template <typename TGS>
void tgsPop(typename TGS::container_type& out_tgs, Token::id_type id) {
	if constexpr (std::is_base_of_v<TGS_Tag_Seq, typename TGS::tag>) {
		auto search = std::find_if(out_tgs.begin(), out_tgs.end(), [id](auto const& entry) -> bool { return entry.first == id; });
		if (search != out_tgs.end()) {
			out_tgs.erase(search);
		}
	} else {
		out_tgs.erase(id);
	}
}
} // namespace detail

inline constexpr Token::Token(detail::TokenGenBase* pParent, ID id) noexcept : pParent(pParent), id(id) {
}
inline constexpr Token::Token(Token&& rhs) noexcept : pParent(rhs.pParent), id(rhs.id) {
	rhs.pParent = nullptr;
	rhs.id = null;
}
inline Token& Token::operator=(Token&& rhs) noexcept {
	if (&rhs != this) {
		if (valid()) {
			pParent->pop(id);
		}
		pParent = std::exchange(rhs.pParent, nullptr);
		id = std::exchange(rhs.id, null);
	}
	return *this;
}
inline Token::~Token() {
	if (valid()) {
		pParent->pop(id);
	}
}
inline constexpr bool Token::valid() const {
	return id != null && pParent != nullptr;
}

inline constexpr Token::ID detail::TokenGenBase::increment(Token::ID id) {
	return id + 1;
}

template <typename T, typename Spec>
struct TTokenGen<T, Spec>::iterator {
	using type = T;
	using pointer = T*;
	using reference = T&;
	using storage = typename Spec::template storage<T>;
	using container_type = typename storage::container_type;

	explicit iterator(typename container_type::iterator iter) : iter(iter) {
	}
	iterator& operator++() {
		++iter;
		return *this;
	}
	reference operator*() {
		return iter->second;
	}
	pointer operator->() {
		return &iter->second;
	}
	friend bool operator==(iterator lhs, iterator rhs) {
		return lhs.iter == rhs.iter;
	}
	friend bool operator!=(iterator lhs, iterator rhs) {
		return lhs.iter != rhs.iter;
	}

  private:
	typename container_type::iterator iter;
};
template <typename T, typename Spec>
struct TTokenGen<T, Spec>::const_iterator {
	using type = T;
	using pointer = T const*;
	using reference = T const&;
	using storage = typename Spec::template storage<T>;
	using container_type = typename storage::container_type;

	explicit const_iterator(typename container_type::const_iterator iter) : iter(iter) {
	}
	const_iterator& operator++() {
		++iter;
		return *this;
	}
	reference operator*() const {
		return iter->second;
	}
	pointer operator->() const {
		return &iter->second;
	}
	friend bool operator==(const_iterator lhs, const_iterator rhs) {
		return lhs.iter == rhs.iter;
	}
	friend bool operator!=(const_iterator lhs, const_iterator rhs) {
		return lhs.iter != rhs.iter;
	}

  private:
	typename container_type::const_iterator iter;
};
template <typename T, typename TGS>
typename TTokenGen<T, TGS>::iterator TTokenGen<T, TGS>::begin() {
	return iterator(m_entries.begin());
}
template <typename T, typename TGS>
typename TTokenGen<T, TGS>::iterator TTokenGen<T, TGS>::end() {
	return iterator(m_entries.end());
}
template <typename T, typename TGS>
typename TTokenGen<T, TGS>::const_iterator TTokenGen<T, TGS>::begin() const {
	return const_iterator(m_entries.begin());
}
template <typename T, typename TGS>
typename TTokenGen<T, TGS>::const_iterator TTokenGen<T, TGS>::end() const {
	return const_iterator(m_entries.end());
}

template <typename T, typename TGS>
template <bool Front, typename U>
Token TTokenGen<T, TGS>::push(U&& u) {
	static_assert(std::is_convertible_v<U, T>, "Invalid argument");
	m_nextID = detail::TokenGenBase::increment(m_nextID);
	Token ret(this, m_nextID);
	detail::tgsPush<Front, storage>(m_entries, m_nextID, std::forward<U>(u));
	return ret;
}
template <typename T, typename TGS>
std::size_t TTokenGen<T, TGS>::clear() noexcept {
	auto const ret = m_entries.size();
	m_entries.clear();
	return ret;
}
template <typename T, typename TGS>
std::size_t TTokenGen<T, TGS>::size() const noexcept {
	return m_entries.size();
}
template <typename T, typename TGS>
bool TTokenGen<T, TGS>::empty() const noexcept {
	return m_entries.empty();
}
template <typename T, typename TGS>
T const* TTokenGen<T, TGS>::find(Token const& token) const {
	return detail::tgsFind<tag, T const>(m_entries, token.id);
}
template <typename T, typename TGS>
T* TTokenGen<T, TGS>::find(Token const& token) {
	return detail::tgsFind<tag, T>(m_entries, token.id);
}
template <typename T, typename TGS>
void TTokenGen<T, TGS>::pop(Token::id_type id) {
	detail::tgsPop<storage>(m_entries, id);
}
} // namespace le
