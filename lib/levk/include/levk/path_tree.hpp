#pragma once
#include <any>
#include <span>
#include <string>
#include <vector>

namespace levk {
struct PathTree {
	std::string identifier{};
	std::any payload{};
	std::vector<PathTree> children{};

	[[nodiscard]] static auto decompose(std::string_view path) -> std::vector<std::string>;

	void add(std::string_view sub_path);
	void add(std::string identifier, std::span<std::string> components);

	template <typename NodeVisitorT>
	void visit(this auto&& self, NodeVisitorT visitor) {
		visitor(self);
		for (auto& child : self.children) { child.visit(visitor); }
	}
};
} // namespace levk
