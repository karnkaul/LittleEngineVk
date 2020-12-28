#pragma once
#include <string>
#include <vector>
#include <core/io/path.hpp>

namespace le {
namespace res {
///
/// \brief List of resource IDs
///
struct ResourceList final {
	std::vector<io::Path> shaders;
	std::vector<io::Path> textures;
	std::vector<io::Path> cubemaps;
	std::vector<io::Path> materials;
	std::vector<io::Path> meshes;
	std::vector<io::Path> models;
	std::vector<io::Path> fonts;

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
