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
template <typename... T>
class PathTree final
{
public:
	using Entry = std::tuple<std::string, T...>;

public:
	struct Node;
	using Nodes = std::vector<Node const*>;

private:
	struct NodeBase
	{
		std::map<std::string, std::unique_ptr<Node>> children;

		Nodes childNodes() const;
		std::size_t childCount() const;
	};

public:
	struct Node : NodeBase
	{
		stdfs::path directory;
		std::map<std::string, Entry> entries;

		Node const* findPattern(std::string_view search, bool bIncludeDirName) const;
	};

private:
	NodeBase m_root;

public:
	void emplace(stdfs::path id, T&&... ts);
	void import(std::vector<Entry> entries);

	std::string print() const;

	Nodes rootNodes() const
	{
		return m_root.childNodes();
	}

private:
	static std::deque<std::string> decompose(stdfs::path dirPath);
	static stdfs::path concatenate(std::deque<std::string> parts);
	void printChildren(NodeBase const& parent, std::stringstream& out_ss, u16 spaces = 0) const;
};

template <typename... T>
auto PathTree<T...>::NodeBase::childNodes() const -> Nodes
{
	Nodes ret;
	ret.reserve(children.size());
	for (auto const& [_, uNode] : children)
	{
		ret.push_back(uNode.get());
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
void PathTree<T...>::emplace(stdfs::path id, T&&... ts)
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
			pNode->entries[entryName] = std::make_tuple(entryName, std::forward<T>(ts)...);
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
std::string PathTree<T...>::print() const
{
	std::stringstream ss;
	printChildren(m_root, ss);
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
void PathTree<T...>::printChildren(NodeBase const& parent, std::stringstream& out_ss, u16 spaces) const
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
