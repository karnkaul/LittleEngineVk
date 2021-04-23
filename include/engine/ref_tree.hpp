#pragma once
#include <forward_list>
#include <core/ensure.hpp>
#include <core/not_null.hpp>

namespace le {
struct RefTreeBase {};

template <typename T, typename Base = RefTreeBase>
class RefTreeRoot : public Base {
  public:
	using type = T;
	using base_t = Base;
	using container_t = std::forward_list<not_null<type*>>;

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
	void addChild(not_null<type*> child);
	bool removeChild(not_null<type*> child) noexcept;

	container_t m_children;
	bool m_root = true;

	template <typename U, typename V>
	friend class RefTreeNode;
};

template <typename T, typename Base = RefTreeBase>
class RefTreeNode : public RefTreeRoot<T, Base> {
  public:
	using type = T;
	using Root = RefTreeRoot<T, Base>;
	using typename Root::base_t;

	RefTreeNode(not_null<Root*> root);
	RefTreeNode(RefTreeNode&& rhs) noexcept;
	RefTreeNode& operator=(RefTreeNode&& rhs) noexcept;
	~RefTreeNode() override;

	type& parent(not_null<Root*> root) noexcept;
	type* parent() noexcept;
	type const* parent() const noexcept;
	Root& root() noexcept;
	Root const& root() const noexcept;

  protected:
	not_null<Root*> m_parent;

  private:
	template <typename U>
	T* cast(U obj) noexcept;
};

// impl

template <typename T, typename Base>
typename RefTreeRoot<T, Base>::container_t const& RefTreeRoot<T, Base>::children() const noexcept {
	return m_children;
}
template <typename T, typename Base>
bool RefTreeRoot<T, Base>::isRoot() const noexcept {
	return m_root;
}

template <typename T, typename Base>
void RefTreeRoot<T, Base>::addChild(not_null<type*> child) {
	if constexpr (levk_debug) {
		for (auto const& ch : m_children) {
			ENSURE(child != ch, "Duplicate child!");
		}
	}
	m_children.push_front(child);
}

template <typename T, typename Base>
bool RefTreeRoot<T, Base>::removeChild(not_null<type*> child) noexcept {
	auto it = m_children.begin();
	if (it == m_children.end()) {
		return false;
	}
	if (*it == child) {
		m_children.pop_front();
		return true;
	}
	for (auto prev = it++; it != m_children.end(); ++it) {
		if (*it == child) {
			m_children.erase_after(prev);
			return true;
		}
		prev = it;
	}
	return false;
}

template <typename T, typename Base>
template <typename U, typename Pred>
void RefTreeRoot<T, Base>::walk(U&& root, Pred pred) {
	static_assert(std::is_base_of_v<RefTreeRoot, std::decay_t<U>>, "Invalid type!");
	for (auto& child : root.m_children) {
		if (pred(*child)) {
			walk(*child, pred);
		}
	}
}

template <typename T, typename Base>
RefTreeNode<T, Base>::RefTreeNode(not_null<Root*> parent) : m_parent(parent) {
	static_assert(std::is_base_of_v<RefTreeNode<T, Base>, T>, "CRTP misuse");
	this->m_root = false;
	parent->addChild(cast(this));
}
template <typename T, typename Base>
RefTreeNode<T, Base>::RefTreeNode(RefTreeNode&& rhs) noexcept : Root(std::move(rhs)), m_parent(rhs.m_parent) {
	this->m_root = false;
	m_parent->addChild(cast(this));
}
template <typename T, typename Base>
RefTreeNode<T, Base>& RefTreeNode<T, Base>::operator=(RefTreeNode&& rhs) noexcept {
	if (&rhs != this) {
		Root::operator=(std::move(rhs));
		if (&rhs.m_parent != &m_parent) {
			m_parent->removeChild(cast(this));
			m_parent = rhs.m_parent;
			m_parent->addChild(cast(this));
		}
	}
	return *this;
}
template <typename T, typename Base>
RefTreeNode<T, Base>::~RefTreeNode() {
	m_parent->removeChild(cast(this));
	for (auto& child : this->m_children) {
		static_cast<type*>(child)->m_parent = m_parent;
		m_parent->addChild(child);
	}
}
template <typename T, typename Base>
T& RefTreeNode<T, Base>::parent(not_null<Root*> parent) noexcept {
	ENSURE(parent.get() != this, "Setting parent to self!");
	if (parent.get() != this && m_parent != parent) {
		m_parent->removeChild(cast(this));
		m_parent = parent;
		parent->addChild(cast(this));
	}
	return *cast(this);
}
template <typename T, typename Base>
RefTreeRoot<T, Base>& RefTreeNode<T, Base>::root() noexcept {
	not_null<RefTreeRoot<T, Base>*> ret = m_parent;
	while (!ret->m_root) {
		ret = cast(ret.get())->m_parent;
	}
	return *ret;
}
template <typename T, typename Base>
RefTreeRoot<T, Base> const& RefTreeNode<T, Base>::root() const noexcept {
	not_null<RefTreeRoot<T, Base> const*> ret = m_parent;
	while (!ret.get().m_root) {
		ret = cast(ret.get())->m_parent;
	}
	return *ret;
}
template <typename T, typename Base>
template <typename U>
T* RefTreeNode<T, Base>::cast(U obj) noexcept {
	return static_cast<T*>(obj);
}
} // namespace le
