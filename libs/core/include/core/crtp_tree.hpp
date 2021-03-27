#pragma once
#include <list>
#include <core/ensure.hpp>
#include <core/ref.hpp>

namespace le {
///
/// \brief CRTP base class for a self-contained N-tree
///
template <typename T>
class CRTPTree {
  public:
	using type = T;
	using container_type = std::list<Ref<type>>;

	///
	/// \brief Default constructor
	///
	constexpr CRTPTree() noexcept;
	///
	/// \brief Move constructor: pilfers rhs and reparents its children
	///
	CRTPTree(CRTPTree<T>&&) noexcept;
	///
	/// \brief Move assignment operator: unparents self, pilfers rhs and reparents its children
	///
	CRTPTree& operator=(CRTPTree<T>&&) noexcept;
	CRTPTree(CRTPTree<T> const&) = delete;
	CRTPTree& operator=(CRTPTree<T> const&) = delete;
	///
	/// \brief Destructor: unparents self and reparents children
	///
	~CRTPTree();

	///
	/// \brief Reparent to pParent (pass `nullptr` to unparent)
	///
	bool parent(T* pParent) noexcept;
	///
	/// \brief Obtain pointer to parent (if exists)
	///
	T* parent() noexcept;
	///
	/// \brief Obtain pointer to parent (if exists)
	///
	T const* parent() const noexcept;
	///
	/// \brief Obtain (const reference to) list of all children
	///
	std::list<Ref<T>> const& children() const noexcept;

	///
	/// \brief Depth-first walk
	/// \param root root node whose children to traverse
	/// \param pred predicate taking `T`, if `false` returned, the sub-tree will be skipped
	///
	template <typename U, typename Pred>
	static void walk(U&& root, Pred pred);

  protected:
	///
	/// \brief List of child trees
	///
	container_type m_children;
	///
	/// \brief Pointer to parent
	///
	T* m_pParent = nullptr;
	///
	/// \brief Set to true on parent changes (otherwise unused in base class)
	///
	mutable bool m_bDirty = false;

  protected:
	void pilfer(T&& rhs);
	void purge();

  private:
	template <typename U, typename Th>
	static constexpr U cast(Th& th) noexcept;
};

// impl

template <typename T>
constexpr CRTPTree<T>::CRTPTree() noexcept {
	static_assert(std::is_base_of_v<CRTPTree<T>, T>, "T must derive from Tree<T>");
}
template <typename T>
CRTPTree<T>::CRTPTree(CRTPTree<T>&& rhs) noexcept
	: m_children(std::move(rhs.m_children)), m_pParent(std::exchange(rhs.m_pParent, nullptr)), m_bDirty(std::exchange(rhs.m_bDirty, false)) {
	pilfer(cast<T&&>(rhs));
}
template <typename T>
CRTPTree<T>& CRTPTree<T>::operator=(CRTPTree<T>&& rhs) noexcept {
	if (&rhs != this) {
		purge();
		m_pParent = std::exchange(rhs.m_pParent, nullptr);
		m_children = std::move(rhs.m_children);
		m_bDirty = std::exchange(rhs.m_bDirty, false);
		pilfer(cast<T&&>(rhs));
	}
	return *this;
}
template <typename T>
CRTPTree<T>::~CRTPTree() {
	purge();
}
template <typename T>
bool CRTPTree<T>::parent(T* pParent) noexcept {
	ENSURE(pParent != this, "Setting parent to self!");
	if (pParent != this && m_pParent != pParent) {
		if (m_pParent) {
			m_pParent->m_children.remove(cast<T&>(*this));
		}
		m_pParent = pParent;
		if (m_pParent) {
			m_pParent->m_children.push_back(cast<T&>(*this));
		}
		m_bDirty = true;
		return true;
	}
	return false;
}
template <typename T>
T* CRTPTree<T>::parent() noexcept {
	return m_pParent;
}
template <typename T>
T const* CRTPTree<T>::parent() const noexcept {
	return m_pParent;
}
template <typename T>
std::list<Ref<T>> const& CRTPTree<T>::children() const noexcept {
	return m_children;
}
template <typename T>
template <typename U, typename Pred>
void CRTPTree<T>::walk(U&& root, Pred pred) {
	static_assert(std::is_base_of_v<CRTPTree<T>, std::decay_t<U>>, "Invalid type!");
	for (T& child : root.m_children) {
		if (pred(child)) {
			walk(child, pred);
		}
	}
}
template <typename T>
template <typename U, typename Th>
constexpr U CRTPTree<T>::cast(Th& th) noexcept {
	return static_cast<U>(th);
}
template <typename T>
void CRTPTree<T>::pilfer(T&& rhs) {
	T& t = cast<T&>(*this);
	if (m_pParent) {
		m_pParent->m_children.remove(rhs);
		m_pParent->m_children.push_back(t);
	}
	for (T& child : m_children) {
		child.m_pParent = std::addressof(t);
		child.m_bDirty = true;
	}
}
template <typename T>
void CRTPTree<T>::purge() {
	if (m_pParent) {
		m_pParent->m_children.remove(cast<T&>(*this));
	}
	for (T& child : m_children) {
		child.m_pParent = m_pParent;
		child.m_bDirty = true;
		if (m_pParent) {
			m_pParent->m_children.push_back(child);
		}
	}
	m_children.clear();
	m_pParent = nullptr;
}
} // namespace le