#pragma once
#include <algorithm>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <core/assert.hpp>
#include <core/std_types.hpp>

namespace le
{
namespace stdfs = std::filesystem;

namespace io
{
///
/// \brief View for a tree of T... associated with a path each
///
template <typename... T>
class PathTree final
{
public:
	///
	/// \brief filename and T...
	///
	using Entry = std::tuple<std::string, T...>;

public:
	struct Node;
	using Nodes = std::vector<Ref<Node const>>;

private:
	struct NodeBase
	{
	protected:
		///
		/// \brief Tree of child Nodes (inaccessible)
		///
		std::map<std::string, std::unique_ptr<Node>> children;

	public:
		///
		/// \brief Obtain all Nodes connected to this Node
		///
		Nodes childNodes() const;
		///
		/// \brief Obtain the count of Nodes connected to this Node
		///
		std::size_t childCount() const;

	private:
		friend class PathTree<T...>;
	};

public:
	///
	/// \brief Node of the path tree (multi-root)
	///
	struct Node : NodeBase
	{
		///
		/// \brief Containing directory
		///
		stdfs::path directory;
		///
		/// \brief Entry list (ordered alphabetically)
		///
		std::map<std::string, Entry> entries;

		///
		/// \brief Search for a string pattern in this Node and its subtree
		///
		Node const* findPattern(std::string_view search, bool bIncludeDirName) const;
	};

private:
	NodeBase m_root;

public:
	///
	/// \brief Add a new Entry
	/// \param id full path
	/// \param ts objects (as per template arguments)
	///
	void emplace(stdfs::path id, T... ts);
	///
	/// \brief Import flat list into tree
	///
	void import(std::vector<Entry> entries);

	///
	/// \brief Obtain depth-first string representation of tree
	///
	std::string print(u8 indent = 2) const;

	///
	/// \brief Obtain all root nodes
	///
	Nodes rootNodes() const
	{
		return m_root.childNodes();
	}

private:
	static std::deque<std::string> decompose(stdfs::path dirPath);
	static stdfs::path concatenate(std::deque<std::string> parts);
	void printChildren(NodeBase const& parent, std::stringstream& out_ss, u8 indent, u8 spaces = 0) const;
};

template <typename... T>
auto PathTree<T...>::NodeBase::childNodes() const -> Nodes
{
	Nodes ret;
	ret.reserve(children.size());
	for (auto const& [_, uNode] : children)
	{
		ret.push_back(*uNode);
	}
	return ret;
}

template <typename... T>
std::size_t PathTree<T...>::NodeBase::childCount() const
{
	return children.size();
}

template <typename... T>
typename PathTree<T...>::Node const* PathTree<T...>::Node::findPattern(std::string_view search, bool bIncludeDirName) const
{
	if (bIncludeDirName && directory.filename().generic_string().find(search) != std::string::npos)
	{
		return this;
	}
	for (auto const& [name, _] : entries)
	{
		if (name.find(search) != std::string::npos)
		{
			return this;
		}
	}
	for (auto const& [name, uNode] : this->children)
	{
		if (auto pRet = uNode->findPattern(search, bIncludeDirName))
		{
			return pRet;
		}
	}
	return nullptr;
}

template <typename... T>
void PathTree<T...>::emplace(stdfs::path id, T... ts)
{
	auto dirs = decompose(std::move(id));
	if (!dirs.empty())
	{
		auto const entryName = dirs.back();
		dirs.pop_back();
		NodeBase* pParent = &m_root;
		Node* pNode = nullptr;
		std::deque<std::string> parts;
		while (!dirs.empty())
		{
			auto const dirName = std::move(dirs.front());
			dirs.pop_front();
			parts.push_back(dirName);
			auto& uNode = pParent->children[dirName];
			if (!uNode)
			{
				uNode = std::make_unique<Node>();
				uNode->directory = concatenate(parts);
			}
			pNode = uNode.get();
			pParent = pNode;
		}
		if (pNode && !entryName.empty())
		{
			ASSERT(pNode->entries.find(entryName) == pNode->entries.end(), "Duplicate entry!");
			pNode->entries[entryName] = std::make_tuple(entryName, std::move(ts)...);
		}
	}
}

template <typename... T>
void PathTree<T...>::import(std::vector<Entry> entries)
{
	for (auto& entry : entries)
	{
		emplace(std::get<stdfs::path>(entry), std::forward<T>(std::get<T>(entry))...);
	}
}

template <typename... T>
std::string PathTree<T...>::print(u8 indent) const
{
	std::stringstream ss;
	printChildren(m_root, ss, indent);
	ss << "\n";
	return ss.str();
}

template <typename... T>
std::deque<std::string> PathTree<T...>::decompose(stdfs::path dirPath)
{
	std::deque<std::string> ret;
	if (!dirPath.empty())
	{
		ret.push_front(dirPath.filename().generic_string());
		while (dirPath.has_parent_path())
		{
			dirPath = dirPath.parent_path();
			ret.push_front(dirPath.filename().generic_string());
		}
	}
	return ret;
}

template <typename... T>
stdfs::path PathTree<T...>::concatenate(std::deque<std::string> parts)
{
	stdfs::path ret;
	for (auto& part : parts)
	{
		ret /= std::move(part);
	}
	return ret;
}

template <typename... T>
void PathTree<T...>::printChildren(NodeBase const& parent, std::stringstream& out_ss, u8 indent, u8 spaces) const
{
	std::string sp(spaces, ' ');
	for (auto const& [dir, uNode] : parent.children)
	{
		out_ss << "\n"
			   << sp << dir << "/ \t"
			   << "[" << uNode->directory << "]";
		std::string const sp_2(spaces + 2, ' ');
		for (auto const& [name, _] : uNode->entries)
		{
			out_ss << "\n" << sp_2 << " - " << name;
		}
		printChildren(*uNode, out_ss, spaces + 2);
	}
}
} // namespace io
} // namespace le
