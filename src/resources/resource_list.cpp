#include <algorithm>
#include <sstream>
#include <engine/resources/resource_list.hpp>

namespace le::res {
namespace {
std::vector<io::Path> intersect(std::vector<io::Path> const& lhs, std::vector<io::Path> const& rhs) {
	std::vector<io::Path> ret;
	for (auto const& id : lhs) {
		auto const search = std::find_if(rhs.begin(), rhs.end(), [&id](io::Path const& rhs) -> bool { return id == rhs; });
		if (search != rhs.end()) {
			ret.push_back(id);
		}
	}
	return ret;
}

std::vector<io::Path> subtract(std::vector<io::Path> const& lhs, std::vector<io::Path> const& rhs) {
	std::vector<io::Path> ret;
	for (auto const& id : lhs) {
		auto const search = std::find_if(rhs.begin(), rhs.end(), [&id](io::Path const& rhs) -> bool { return id == rhs; });
		if (search == rhs.end()) {
			ret.push_back(id);
		}
	}
	return ret;
}

template <typename... T>
constexpr bool allEmpty(T const&... t) {
	return (t.empty() && ...);
}

template <typename... T>
constexpr std::size_t totalSize(T const&... t) {
	return (... + t.size());
}
} // namespace

ResourceList ResourceList::operator*(const ResourceList& rhs) const {
	ResourceList ret;
	ret.shaders = intersect(shaders, rhs.shaders);
	ret.textures = intersect(textures, rhs.textures);
	ret.cubemaps = intersect(cubemaps, rhs.cubemaps);
	ret.materials = intersect(materials, rhs.materials);
	ret.meshes = intersect(meshes, rhs.meshes);
	ret.models = intersect(models, rhs.models);
	ret.fonts = intersect(fonts, rhs.fonts);
	return ret;
}

ResourceList ResourceList::operator-(const ResourceList& rhs) const {
	ResourceList ret;
	ret.shaders = subtract(shaders, rhs.shaders);
	ret.textures = subtract(textures, rhs.textures);
	ret.cubemaps = subtract(cubemaps, rhs.cubemaps);
	ret.materials = subtract(materials, rhs.materials);
	ret.meshes = subtract(meshes, rhs.meshes);
	ret.models = subtract(models, rhs.models);
	ret.fonts = subtract(fonts, rhs.fonts);
	return ret;
}

bool ResourceList::empty() const {
	return allEmpty(shaders, textures, cubemaps, materials, meshes, models, fonts);
}

std::size_t ResourceList::size() const {
	return totalSize(shaders, textures, cubemaps, materials, meshes, models, fonts);
}

std::string ResourceList::print() const {
	std::stringstream ret;
	auto add = [&ret](std::vector<io::Path> const& vec, std::string_view title) {
		ret << title << "\n";
		for (auto const& id : vec) {
			ret << "\t" << id.generic_string() << "\n";
		}
	};
	add(shaders, "Shaders");
	add(textures, "Textures");
	add(cubemaps, "Cubemaps");
	add(materials, "Materials");
	add(meshes, "Meshes");
	add(models, "Models");
	add(fonts, "Fonts");
	return ret.str();
}
} // namespace le::res
