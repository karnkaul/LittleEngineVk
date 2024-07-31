#include <levk/path_tree.hpp>
#include <filesystem>

namespace levk {
namespace fs = std::filesystem;

namespace {
[[nodiscard]] auto do_decompose(fs::path path) {
	auto ret = std::vector<std::string>{};
	if (path.is_absolute()) { return ret; }
	for (; !path.empty() && path != path.parent_path(); path = path.parent_path()) { ret.push_back(path.filename().generic_string()); }
	std::ranges::reverse(ret);
	return ret;
}
} // namespace

auto PathTree::decompose(std::string_view path) -> std::vector<std::string> { return do_decompose(path); }

void PathTree::add(std::string_view sub_path) {
	auto ids = decompose(sub_path);
	auto components = std::span{ids};
	if (components.empty()) { return; }
	auto identifier = std::move(components.front());
	components = components.subspan(1);
	add(std::move(identifier), components);
}

void PathTree::add(std::string identifier, std::span<std::string> components) {
	auto it = std::ranges::find_if(children, [&identifier](PathTree const& p) { return p.identifier == identifier; });
	if (it == children.end()) { it = children.insert(children.end(), PathTree{.identifier = std::move(identifier)}); }
	if (components.empty()) { return; }
	identifier = std::move(components.front());
	components = components.subspan(1);
	it->add(std::move(identifier), components);
}
} // namespace levk
