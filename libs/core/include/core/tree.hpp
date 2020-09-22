#pragma once
#include <list>
#include <core/std_types.hpp>
#include <core/assert.hpp>

namespace le
{
///
/// \brief CRTP base class for a self-contained N-tree
///
template <typename T>
class Tree
{
public:
	///
	/// \brief Default constructor
	///
	constexpr Tree() noexcept;
	///
	/// \brief Move constructor: pilfers rhs and reparents its children
	///
	Tree(Tree<T>&&) noexcept;
	///
	/// \brief Move assignment operator: unparents self, pilfers rhs and reparents its children
	///
	Tree& operator=(Tree<T>&&) noexcept;
	Tree(Tree<T> const&) = delete;
	Tree& operator=(Tree<T> const&) = delete;
	///
	/// \brief Destructor: unparents self and reparents children
	///
	~Tree();

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
	/// \param pred predicate taking `T`, if `false` returned, the sub-tree will be skipped
	///
	template <typename U, typename Pred>
	static void walk(U&& root, Pred pred);

protected:
	///
	/// \brief List of child trees
	///
	std::list<Ref<T>> m_children;
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
	static constexpr U& cast(Th& th) noexcept;
};

template <typename T>
constexpr Tree<T>::Tree() noexcept
{
	static_assert(std::is_base_of_v<Tree<T>, T>, "T must derive from Tree<T>");
}

template <typename T>
Tree<T>::Tree(Tree<T>&& rhs) noexcept : m_pParent(rhs.m_pParent), m_children(std::move(rhs.m_children))
{
	pilfer(std::move(cast<T&&>(rhs)));
}

template <typename T>
Tree<T>& Tree<T>::operator=(Tree<T>&& rhs) noexcept
{
	if (&rhs != this)
	{
		purge();
		m_pParent = rhs.m_pParent;
		m_children = std::move(rhs.m_children);
		pilfer(std::move(cast<T&&>(rhs)));
	}
	return *this;
}

template <typename T>
Tree<T>::~Tree()
{
	purge();
}

template <typename T>
bool Tree<T>::parent(T* pParent) noexcept
{
	ASSERT(pParent != this, "Setting parent to self!");
	if (pParent != this && m_pParent != pParent)
	{
		if (m_pParent)
		{
			m_pParent->m_children.remove(cast<T&>(*this));
		}
		m_pParent = pParent;
		if (m_pParent)
		{
			m_pParent->m_children.push_back(cast<T&>(*this));
		}
		m_bDirty = true;
		return true;
	}
	return false;
}

template <typename T>
T* Tree<T>::parent() noexcept
{
	return m_pParent;
}

template <typename T>
T const* Tree<T>::parent() const noexcept
{
	return m_pParent;
}

template <typename T>
std::list<Ref<T>> const& Tree<T>::children() const noexcept
{
	return m_children;
}

template <typename T>
template <typename U, typename Pred>
void Tree<T>::walk(U&& root, Pred pred)
{
	static_assert(std::is_base_of_v<Tree<T>, std::decay_t<T>>, "Invalid type!");
	for (T& child : root.m_children)
	{
		if (pred(child))
		{
			walk(child, pred);
		}
	}
}

template <typename T>
template <typename U, typename Th>
constexpr U& Tree<T>::cast(Th& th) noexcept
{
	return static_cast<U&>(th);
}

template <typename T>
void Tree<T>::pilfer(T&& rhs)
{
	T& t = cast<T&>(*this);
	rhs.m_pParent = nullptr;
	if (m_pParent)
	{
		m_pParent->m_children.remove(rhs);
		m_pParent->m_children.push_back(t);
	}
	for (T& child : m_children)
	{
		child.m_pParent = std::addressof(t);
		child.m_bDirty = true;
	}
}

template <typename T>
void Tree<T>::purge()
{
	if (m_pParent)
	{
		m_pParent->m_children.remove(cast<T&>(*this));
	}
	for (T& child : m_children)
	{
		child.m_pParent = m_pParent;
		child.m_bDirty = true;
		if (m_pParent)
		{
			m_pParent->m_children.push_back(child);
		}
	}
	m_children.clear();
	m_pParent = nullptr;
}
} // namespace le
