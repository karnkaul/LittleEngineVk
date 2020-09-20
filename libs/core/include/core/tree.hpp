#pragma once
#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>
#include <core/assert.hpp>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Default type representing a concrete Node, derives from TNodeBase
///
template <typename T>
struct TNode;

///
/// \brief Wrapper type for entire, multi-root tree
///
template <typename T, template <typename> typename Node = TNode>
struct TTree;

///
/// \brief Default type representing a root node; TNode derives from this
///
template <typename T, template <typename> typename Node>
struct TNodeBase
{
	using type = Node<T>;
	using value_type = T;

	///
	/// \brief Container of this children of this node
	///
	std::vector<std::unique_ptr<Node<T>>> children;

	///
	/// \brief Constructor
	///
	template <typename... Args>
	Node<T>& push(Args&&... args);
};

template <typename T>
struct TNode : TNodeBase<T, TNode>
{
	///
	/// \brief typedef exported for TTree
	///
	using base_type = TNodeBase<T, TNode>;
	static constexpr bool noexcept_v = std::is_trivially_constructible_v<T>;

	///
	/// \brief Reference to parent TNodeBase
	///
	Ref<base_type> parent_;
	///
	/// \brief Stored payload
	///
	T payload;

	///
	/// \brief Constructor
	///
	template <typename... Args>
	constexpr TNode(base_type& parent, Args&&... args) noexcept(noexcept(noexcept_v));

	///
	/// \brief API function for TTree
	///
	constexpr base_type& parent();
};

template <typename T, template <typename> typename Node>
struct TTree
{
	using type = Node<T>;
	using value_type = T;
	using base_type = typename type::base_type;

	///
	/// \brief Root node of the tree (use TNodeBase by default)
	///
	base_type root;

	///
	/// \brief Add a new child node to the parent
	///
	template <typename... Args>
	static type& push(base_type& parent, Args&&...);

	///
	/// \brief Remove a node (and all its leaves) from its parent
	///
	void pop(type& node);
};

template <typename T, template <typename> typename Node>
template <typename... Args>
Node<T>& TNodeBase<T, Node>::push(Args&&... args)
{
	children.push_back(std::make_unique<Node<T>>(*this, std::forward<Args>(args)...));
	return *children.back();
}

template <typename T>
template <typename... Args>
constexpr TNode<T>::TNode(base_type& parent, Args&&... args) noexcept(noexcept(noexcept_v)) : parent_(parent), payload(std::forward<Args>(args)...)
{
}

template <typename T>
constexpr typename TNode<T>::base_type& TNode<T>::parent()
{
	return parent_;
}

template <typename T, template <typename> typename Node>
template <typename... Args>
typename TTree<T, Node>::type& TTree<T, Node>::push(base_type& parent, Args&&... args)
{
	return parent.push(std::forward<Args>(args)...);
}

template <typename T, template <typename> typename Node>
void TTree<T, Node>::pop(type& node)
{
	base_type& p = node.parent();
	auto search = std::remove_if(p.children.begin(), p.children.end(), [&node](auto const& c) { return c.get() == &node; });
	ASSERT(search != p.children.end(), "Invariant violated");
	p.children.erase(search, p.children.end());
}
} // namespace le
