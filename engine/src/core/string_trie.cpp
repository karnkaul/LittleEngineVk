#include <le/core/string_trie.hpp>
#include <algorithm>
#include <cassert>

namespace le {
void StringTrie::Node::append(std::string_view const tail) { // NOLINT(misc-no-recursion)
	if (tail.empty()) { return; }

	auto it = std::ranges::find_if(children, [c = tail.front()](Node const& n) { return c == n.key; });
	if (it == children.end()) {
		children.push_back(Node{.key = tail.front()});
		it = children.end() - 1;
	}

	it->append(tail.substr(1));
}

// NOLINTNEXTLINE(misc-no-recursion)
auto StringTrie::Node::find(std::string_view tail) const -> Ptr<Node const> {
	if (tail.empty()) { return this; } // match

	if (is_leaf()) { return nullptr; } // no children to match tail

	auto const it = std::ranges::find_if(children, [c = tail.front()](Node const& n) { return n.key == c; });
	if (it == children.end()) { return nullptr; }

	return it->find(tail.substr(1));
}

// NOLINTNEXTLINE(misc-no-recursion)
void StringTrie::Node::add_words(std::vector<std::string>& out, std::string_view const prefix) const {
	auto const next_prefix = key == '\0' ? std::string{prefix} : std::format("{}{}", prefix, key);

	// no children, early returns
	if (is_leaf()) {
		// don't push an empty word
		if (next_prefix.empty()) { return; }

		// end of word, push it
		return out.push_back(next_prefix);
	}

	// add all possible words from here
	for (auto const& node : children) { node.add_words(out, next_prefix); }
}

StringTrie::StringTrie(std::initializer_list<std::string_view> const& words) {
	for (auto const word : words) { add(word); }
}

void StringTrie::add_candidates_to(std::vector<std::string>& out, std::string_view fragment) const {
	auto const prefix = fragment.empty() ? fragment : fragment.substr(0, fragment.size() - 1);
	if (auto const* node = m_root.find(fragment)) { node->add_words(out, prefix); }
}

auto StringTrie::build_candidates(std::string_view fragment) const -> std::vector<std::string> {
	auto ret = std::vector<std::string>{};
	add_candidates_to(ret, fragment);
	return ret;
}
} // namespace le
