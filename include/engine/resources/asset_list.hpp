#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace le
{
namespace stdfs = std::filesystem;

struct AssetList final
{
	std::vector<stdfs::path> shaders;
	std::vector<stdfs::path> textures;
	std::vector<stdfs::path> cubemaps;
	std::vector<stdfs::path> materials;
	std::vector<stdfs::path> meshes;
	std::vector<stdfs::path> models;
	std::vector<stdfs::path> fonts;

	AssetList operator*(AssetList const& rhs) const;
	AssetList operator-(AssetList const& rhs) const;

	bool isEmpty() const;

	std::string print() const;
};
} // namespace le
