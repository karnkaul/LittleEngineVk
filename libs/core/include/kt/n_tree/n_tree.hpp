// KT header-only library
// Requirements: C++17

#pragma once
#include <forward_list>
#include <type_traits>

namespace kt {
///
/// \brief Models a "forward" N-tree (no parent link) via std::forward_list
///
template <typename T>
class n_tree {
	static_assert(!std::is_reference_v<T>, "Reference types are not supported!");

	template <typename... U>
	static constexpr bool is_diff = (!std::is_same_v<std::decay_t<U>, n_tree<T>> && ...);
	template <typename... U>
	using enable_if_diff = std::enable_if_t<is_diff<U...>>;

  public:
	using value_type = T;
	using storage_t = std::forward_list<n_tree>;

	///
	/// \brief Variadic perfect-forwarding constructor
	///
	template <typename... U, typename = enable_if_diff<U...>>
	explicit n_tree(U&&... u) noexcept;

	///
	/// \brief Move t to front of children
	///
	n_tree& push_front(T&& t);
	///
	/// \brief Copy t to front of children
	///
	n_tree& push_front(T const& t);
	///
	/// \brief Destroy all children
	///
	void clear_children() noexcept;
	///
	/// \brief Erase node if child (recursive)
	///
	bool erase_child(n_tree const& node) noexcept;

	///
	/// \brief Check if this node has any child nodes
	///
	bool has_children() const noexcept;
	///
	/// \brief Obtain (a const reference to) all child nodes
	///
	storage_t const& children() const noexcept;

	///
	/// \brief Perform a DFS using a predicate (called with T passed)
	///
	template <typename Pred>
	n_tree* depth_first_find(Pred pred) noexcept;
	///
	/// \brief Perform a DFS using a predicate (called with T passed)
	///
	template <typename Pred>
	n_tree const* depth_first_find(Pred pred) const noexcept;

	///
	/// \brief Payload
	///
	T m_t;

  private:
	storage_t m_children;
};

// impl

template <typename T>
template <typename... U, typename>
n_tree<T>::n_tree(U&&... u) noexcept : m_t(std::forward<U>(u)...) {
	static_assert(std::is_constructible_v<T, U...>, "Invalid arguments");
}
template <typename T>
n_tree<T>& n_tree<T>::push_front(T&& t) {
	m_children.emplace_front(std::move(t));
	return *m_children.begin();
}
template <typename T>
n_tree<T>& n_tree<T>::push_front(T const& t) {
	m_children.emplace_front(t);
	return *m_children.begin();
}
template <typename T>
void n_tree<T>::clear_children() noexcept {
	m_children.clear();
}
template <typename T>
bool n_tree<T>::erase_child(n_tree const& node) noexcept {
	if (!has_children()) { return false; }
	// First node is a special case (cannot erase_after())
	auto it = m_children.begin();
	if (&*it == &node) {
		m_children.pop_front();
		return true;
	}
	// Depth first
	if (it->erase_child(node)) { return true; }
	// Other nodes are general cases
	for (auto prev = it++; it != m_children.end(); ++it) {
		if (&*it == &node) {
			m_children.erase_after(prev);
			return true;
		}
		// Depth first
		if (it->erase_child(node)) { return true; }
		prev = it;
	}
	return false;
}
template <typename T>
bool n_tree<T>::has_children() const noexcept {
	return !m_children.empty();
}
template <typename T>
typename n_tree<T>::storage_t const& n_tree<T>::children() const noexcept {
	return m_children;
}
template <typename T>
template <typename Pred>
n_tree<T>* n_tree<T>::depth_first_find(Pred pred) noexcept {
	for (auto& child : m_children) {
		if (auto ret = child.find(pred)) { return ret; }
	}
	if (pred(m_t)) { return this; }
	return nullptr;
}
template <typename T>
template <typename Pred>
n_tree<T> const* n_tree<T>::depth_first_find(Pred pred) const noexcept {
	for (auto const& child : m_children) {
		if (auto const ret = child.find(pred)) { return ret; }
	}
	if (pred(m_t)) { return this; }
	return nullptr;
}
} // namespace kt
