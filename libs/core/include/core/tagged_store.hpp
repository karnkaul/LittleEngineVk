#pragma once
#include <cstdint>
#include <unordered_map>

namespace le {
using u64 = std::uint64_t;

///
/// \brief RAII Tag (moveable) that will remove its corresponding entry on destruction
///
/// Important: Owning TaggedStore instance must outlive all valid Tag instances
///
template <typename Id = u64>
struct Tag;
///
/// \brief Customization point for storage of tagged Ty instances
///
template <typename Ty, typename Id>
struct TaggedMap;
///
/// \brief Wrapper for a container of Ty associated with RAII tags
///
/// Important: Owning TaggedStore instance must outlive all valid Tag instances
///
template <typename Ty, typename Tg = Tag<>, typename St = TaggedMap<Ty, typename Tg::type>>
class TaggedStore;

namespace detail {
template <typename Tg>
struct TaggedStoreBase;
} // namespace detail

///
/// \brief RAII Tag (moveable) that will remove its corresponding entry on destruction
///
/// Important: Owning TaggedStore instance must outlive all valid Tag instances
///
template <typename Id>
struct Tag final {
	using type = Id;
	static constexpr type null = Id{};

	constexpr Tag() = default;
	constexpr Tag(detail::TaggedStoreBase<Tag>* parent, type id) noexcept;
	constexpr Tag(Tag&& rhs) noexcept;
	Tag& operator=(Tag&& rhs) noexcept;
	~Tag();

	///
	/// \brief Increment passed type by one
	///
	static constexpr type increment(type t) noexcept;
	///
	/// \brief Check if this instance is valid
	///
	constexpr bool valid() const noexcept;

  private:
	detail::TaggedStoreBase<Tag>* store = nullptr;
	type id = null;

	template <typename Ty, typename Tg, typename St>
	friend class TaggedStore;
};

///
/// \brief Customization point for storage of tagged T instances
///
/// All aliases, data members, and member functions are required
/// Provide push_front(id, U&&...) if applicable
///
template <typename Ty, typename Id>
struct TaggedMap {
	using type = Ty;
	using id_t = Id;
	using container_t = std::unordered_map<id_t, type>;

	container_t storage;

	template <typename... U>
	void push_back(id_t id, U&&... u);
	void pop(id_t id) noexcept;
	type* find(id_t id) noexcept;
	type const* find(id_t id) const noexcept;
};

namespace detail {
///
/// \brief Base class for Tags to store a pointer to (to call pop() on destruction)
///
template <typename Tg>
struct TaggedStoreBase {
	using tag_t = Tg;
	using id_t = typename tag_t::type;

	virtual void pop(id_t id) noexcept = 0;
};
} // namespace detail

///
/// \brief Wrapper for a container of T associated with RAII tags
///
/// Important: Owning TaggedStore instance must outlive all valid Tag instances
///
template <typename Ty, typename Tg, typename St>
class TaggedStore final : detail::TaggedStoreBase<Tg> {
  public:
	using type = Ty;
	using storage_t = St;
	using tag_t = Tg;
	using container_t = typename storage_t::container_t;

	using iterator = typename container_t::iterator;
	using reverse_iterator = typename container_t::reverse_iterator;
	using const_iterator = typename container_t::const_iterator;
	using const_reverse_iterator = typename container_t::const_reverse_iterator;

	///
	/// \brief Add a new entry
	/// \returns Tag (moveable) that will remove entry on destruction
	///
	template <typename... U>
	tag_t emplace_back(U&&... u);
	///
	/// \brief Add a new entry
	/// \returns Tag (moveable) that will remove entry on destruction
	///
	template <typename... U>
	tag_t emplace_front(U&&... u);
	///
	/// \brief Search for a T mapped to a tag
	///
	type const* find(tag_t const& tag) const noexcept;
	///
	/// \brief Search for a T mapped to a tag
	///
	type* find(tag_t const& tag) noexcept;
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

	iterator begin();
	iterator end();
	const_iterator cbegin();
	const_iterator cend();
	const_iterator begin() const;
	const_iterator end() const;
	reverse_iterator rbegin();
	reverse_iterator rend();
	const_reverse_iterator rbegin() const;
	const_reverse_iterator rend() const;

  private:
	using id_t = typename tag_t::type;

	void pop(id_t id) noexcept override;

	storage_t m_storage;
	id_t m_nextID = tag_t::null;
};

// impl

template <typename T>
constexpr Tag<T>::Tag(detail::TaggedStoreBase<Tag>* parent, type id) noexcept : store(parent), id(id) {
}
template <typename T>
constexpr Tag<T>::Tag(Tag&& rhs) noexcept : store(rhs.store), id(rhs.id) {
	rhs.store = nullptr;
	rhs.id = null;
}
template <typename T>
Tag<T>& Tag<T>::operator=(Tag&& rhs) noexcept {
	if (&rhs != this) {
		if (valid()) {
			store->pop(id);
		}
		store = std::exchange(rhs.store, nullptr);
		id = std::exchange(rhs.id, null);
	}
	return *this;
}
template <typename T>
Tag<T>::~Tag() {
	if (valid()) {
		store->pop(id);
	}
}
template <typename T>
constexpr typename Tag<T>::type Tag<T>::increment(T t) noexcept {
	return ++t;
}
template <typename T>
constexpr bool Tag<T>::valid() const noexcept {
	return id != null && store != nullptr;
}

template <typename Ty, typename Id>
template <typename... U>
void TaggedMap<Ty, Id>::push_back(id_t id, U&&... u) {
	storage.emplace(id, std::forward<U>(u)...);
}
template <typename Ty, typename Id>
void TaggedMap<Ty, Id>::pop(id_t id) noexcept {
	storage.erase(id);
}
template <typename Ty, typename Id>
Ty* TaggedMap<Ty, Id>::find(id_t id) noexcept {
	if (auto const it = storage.find(id); it != storage.end()) {
		return &it->second;
	}
	return nullptr;
}
template <typename Ty, typename Id>
Ty const* TaggedMap<Ty, Id>::find(id_t id) const noexcept {
	if (auto const it = storage.find(id); it != storage.end()) {
		return &it->second;
	}
	return nullptr;
}

template <typename T, typename Tg, typename St>
typename TaggedStore<T, Tg, St>::iterator TaggedStore<T, Tg, St>::begin() {
	return m_storage.storage.begin();
}
template <typename T, typename Tg, typename St>
typename TaggedStore<T, Tg, St>::iterator TaggedStore<T, Tg, St>::end() {
	return m_storage.storage.end();
}
template <typename T, typename Tg, typename St>
typename TaggedStore<T, Tg, St>::const_iterator TaggedStore<T, Tg, St>::cbegin() {
	return m_storage.storage.begin();
}
template <typename T, typename Tg, typename St>
typename TaggedStore<T, Tg, St>::const_iterator TaggedStore<T, Tg, St>::cend() {
	return m_storage.storage.end();
}
template <typename T, typename Tg, typename St>
typename TaggedStore<T, Tg, St>::const_iterator TaggedStore<T, Tg, St>::begin() const {
	return m_storage.storage.begin();
}
template <typename T, typename Tg, typename St>
typename TaggedStore<T, Tg, St>::const_iterator TaggedStore<T, Tg, St>::end() const {
	return m_storage.storage.end();
}
template <typename T, typename Tg, typename St>
typename TaggedStore<T, Tg, St>::reverse_iterator TaggedStore<T, Tg, St>::rbegin() {
	return m_storage.storage.rbegin();
}
template <typename T, typename Tg, typename St>
typename TaggedStore<T, Tg, St>::reverse_iterator TaggedStore<T, Tg, St>::rend() {
	return m_storage.storage.rend();
}

template <typename T, typename Tg, typename St>
typename TaggedStore<T, Tg, St>::const_reverse_iterator TaggedStore<T, Tg, St>::rbegin() const {
	return m_storage.storage.rbegin();
}
template <typename T, typename Tg, typename St>
typename TaggedStore<T, Tg, St>::const_reverse_iterator TaggedStore<T, Tg, St>::rend() const {
	return m_storage.storage.rend();
}

template <typename T, typename Tg, typename St>
template <typename... U>
Tg TaggedStore<T, Tg, St>::emplace_back(U&&... u) {
	m_nextID = tag_t::increment(m_nextID);
	tag_t ret(this, m_nextID);
	m_storage.push_back(m_nextID, std::forward<U>(u)...);
	return ret;
}
template <typename T, typename Tg, typename St>
template <typename... U>
Tg TaggedStore<T, Tg, St>::emplace_front(U&&... u) {
	m_nextID = tag_t::increment(m_nextID);
	tag_t ret(this, m_nextID);
	m_storage.push_front(m_nextID, std::forward<U>(u)...);
	return ret;
}
template <typename T, typename Tg, typename St>
T const* TaggedStore<T, Tg, St>::find(tag_t const& tag) const noexcept {
	return m_storage.find(tag.id);
}
template <typename T, typename Tg, typename St>
T* TaggedStore<T, Tg, St>::find(tag_t const& tag) noexcept {
	return m_storage.find(tag.id);
}
template <typename T, typename Tg, typename St>
std::size_t TaggedStore<T, Tg, St>::clear() noexcept {
	auto const ret = m_storage.storage.size();
	m_storage.storage.clear();
	return ret;
}
template <typename T, typename Tg, typename St>
std::size_t TaggedStore<T, Tg, St>::size() const noexcept {
	return m_storage.storage.size();
}
template <typename T, typename Tg, typename St>
bool TaggedStore<T, Tg, St>::empty() const noexcept {
	return m_storage.storage.empty();
}
template <typename T, typename Tg, typename St>
void TaggedStore<T, Tg, St>::pop(id_t id) noexcept {
	m_storage.pop(id);
}
} // namespace le
