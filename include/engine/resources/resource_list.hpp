#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace le {
namespace stdfs = std::filesystem;

namespace res {
///
/// \brief List of resource IDs
///
struct ResourceList final {
	std::vector<stdfs::path> shaders;
	std::vector<stdfs::path> textures;
	std::vector<stdfs::path> cubemaps;
	std::vector<stdfs::path> materials;
	std::vector<stdfs::path> meshes;
	std::vector<stdfs::path> models;
	std::vector<stdfs::path> fonts;

	///
	/// \brief Intersect with `rhs`
	///
	ResourceList operator*(ResourceList const& rhs) const;
	///
	/// \brief Subtract intersection of `rhs`
	///
	ResourceList operator-(ResourceList const& rhs) const;

	///
	/// \brief Check if any resourceIDs are present
	///
	bool empty() const;
	///
	/// \brief Obtain a count of all resource IDs
	///
	std::size_t size() const;

	///
	/// \brief Obtain string output of entire list
	///
	std::string print() const;
};
} // namespace res
} // namespace le
