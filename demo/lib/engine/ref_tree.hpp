#pragma once
#include <forward_list>
#include <core/ensure.hpp>
#include <core/ref.hpp>

namespace le {

template <typename T>
class RefTreeRoot {
  public:
	using type = T;
	using container_t = std::forward_list<Ref<type>>;

	RefTreeRoot() = default;
	RefTreeRoot(RefTreeRoot&&) = default;
	RefTreeRoot& operator=(RefTreeRoot&&) = default;
	RefTreeRoot(RefTreeRoot const&) = delete;
	RefTreeRoot& operator=(RefTreeRoot const&) = delete;
	virtual ~RefTreeRoot() = default;

	container_t const& children() const noexcept;
	bool isRoot() const noexcept;

	///
	/// \brief Depth-first walk
	/// \param root root node whose children to traverse
	/// \param pred predicate taking `T`, if `false` returned, the sub-tree will be skipped
	///
	template <typename U, typename Pred>
	static void walk(U&& root, Pred pred);

  private:
	bool addChild(type& child);
	bool removeChild(type& child) noexcept;

	container_t m_children;
	bool m_root = true;

	template <typename U>
	friend class RefTreeNode;
};

template <typename T>
class RefTreeNode : public RefTreeRoot<T> {
  public:
	using type = T;
	using Root = RefTreeRoot<T>;

	RefTreeNode(Root& root);
	RefTreeNode(RefTreeNode&& rhs) noexcept;
	RefTreeNode& operator=(RefTreeNode&& rhs) noexcept;
	~RefTreeNode() override;

	type& parent(Root& root) noexcept;
	type* parent() noexcept;
	type const* parent() const noexcept;
	Root& root() noexcept;
	Root const& root() const noexcept;

  protected:
	Ref<Root> m_parent;

  private:
	template <typename U>
	T& cast(U& obj) noexcept;
};

// impl

template <typename T>
typename RefTreeRoot<T>::container_t const& RefTreeRoot<T>::children() const noexcept {
	return m_children;
}
template <typename T>
bool RefTreeRoot<T>::isRoot() const noexcept {
	return m_root;
}

template <typename T>
bool RefTreeRoot<T>::addChild(type& child) {
	for (auto const& ch : m_children) {
		if (&child == &ch.get()) {
			return false;
		}
	}
	m_children.push_front(child);
	return true;
}

template <typename T>
bool RefTreeRoot<T>::removeChild(type& child) noexcept {
	auto it = m_children.begin();
	if (it == m_children.end()) {
		return false;
	}
	if (&it->get() == &child) {
		m_children.pop_front();
		return true;
	}
	for (auto prev = it++; it != m_children.end(); ++it) {
		if (&it->get() == &child) {
			m_children.erase_after(prev);
			return true;
		}
		prev = it;
	}
	return false;
}

template <typename T>
template <typename U, typename Pred>
void RefTreeRoot<T>::walk(U&& root, Pred pred) {
	static_assert(std::is_base_of_v<RefTreeRoot, std::decay_t<U>>, "Invalid type!");
	for (auto& child : root.m_children) {
		if (pred(child)) {
			walk(child, pred);
		}
	}
}

template <typename T>
RefTreeNode<T>::RefTreeNode(Root& parent) : m_parent(parent) {
	static_assert(std::is_base_of_v<RefTreeNode<T>, T>, "CRTP misuse");
	this->m_root = false;
	parent.addChild(cast(*this));
}
template <typename T>
RefTreeNode<T>::RefTreeNode(RefTreeNode&& rhs) noexcept : Root(std::move(rhs)), m_parent(rhs.m_parent) {
	this->m_root = false;
	m_parent.get().addChild(cast(*this));
}
template <typename T>
RefTreeNode<T>& RefTreeNode<T>::operator=(RefTreeNode&& rhs) noexcept {
	if (&rhs != this) {
		Root::operator=(std::move(rhs));
		if (&rhs.m_parent.get() != &m_parent.get()) {
			m_parent.get().removeChild(cast(*this));
			m_parent = rhs.m_parent;
			m_parent.get().addChild(cast(*this));
		}
	}
	return *this;
}
template <typename T>
RefTreeNode<T>::~RefTreeNode() {
	m_parent.get().removeChild(cast(*this));
	for (auto& child : this->m_children) {
		static_cast<type&>(child).m_parent = m_parent;
		m_parent.get().addChild(child);
	}
}
template <typename T>
T& RefTreeNode<T>::parent(Root& parent) noexcept {
	ENSURE(&parent != this, "Setting parent to self!");
	if (&parent != this && &m_parent.get() != &parent) {
		m_parent.get().removeChild(cast(*this));
		m_parent = parent;
		parent.addChild(cast(*this));
	}
	return cast(*this);
}
template <typename T>
RefTreeRoot<T>& RefTreeNode<T>::root() noexcept {
	Ref<RefTreeRoot<T>> ret = m_parent;
	while (!ret.get().m_root) {
		ret = cast(ret.get()).m_parent;
	}
	return ret;
}
template <typename T>
RefTreeRoot<T> const& RefTreeNode<T>::root() const noexcept {
	Ref<RefTreeRoot<T> const> ret = m_parent;
	while (!ret.get().m_root) {
		ret = cast(ret.get()).m_parent;
	}
	return ret;
}
template <typename T>
template <typename U>
T& RefTreeNode<T>::cast(U& obj) noexcept {
	return static_cast<T&>(obj);
}
} // namespace le
