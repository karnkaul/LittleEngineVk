#pragma once
#include <le/core/ptr.hpp>
#include <string>
#include <vector>

namespace le {
class StringTrie {
  public:
	StringTrie() = default;

	explicit StringTrie(std::initializer_list<std::string_view> const& words);

	void add(std::string_view word) { m_root.append(word); }
	void add_candidates_to(std::vector<std::string>& out, std::string_view fragment) const;
	[[nodiscard]] auto build_candidates(std::string_view fragment) const -> std::vector<std::string>;

  private:
	struct Node {
		char key{};
		std::vector<Node> children{};

		void append(std::string_view tail);
		[[nodiscard]] auto find(std::string_view tail) const -> Ptr<Node const>;
		void add_words(std::vector<std::string>& out, std::string_view prefix) const;

		[[nodiscard]] auto is_leaf() const { return children.empty(); }
	};

	Node m_root{};
};
} // namespace le
