#include <algorithm>
#include <engine/resources/asset_list.hpp>

namespace le
{
namespace
{
std::vector<stdfs::path> intersect(std::vector<stdfs::path> const& lhs, std::vector<stdfs::path> const& rhs)
{
	std::vector<stdfs::path> ret;
	for (auto const& id : lhs)
	{
		auto const search = std::find_if(rhs.begin(), rhs.end(), [&id](stdfs::path const& rhs) -> bool { return id == rhs; });
		if (search != rhs.end())
		{
			ret.push_back(id);
		}
	}
	return ret;
}

std::vector<stdfs::path> subtract(std::vector<stdfs::path> const& lhs, std::vector<stdfs::path> const& rhs)
{
	std::vector<stdfs::path> ret;
	for (auto const& id : lhs)
	{
		auto const search = std::find_if(rhs.begin(), rhs.end(), [&id](stdfs::path const& rhs) -> bool { return id == rhs; });
		if (search == rhs.end())
		{
			ret.push_back(id);
		}
	}
	return ret;
}
} // namespace

AssetList AssetList::operator*(const AssetList& rhs) const
{
	AssetList ret;
	ret.shaders = intersect(shaders, rhs.shaders);
	ret.textures = intersect(textures, rhs.textures);
	ret.cubemaps = intersect(cubemaps, rhs.cubemaps);
	ret.materials = intersect(materials, rhs.materials);
	ret.meshes = intersect(meshes, rhs.meshes);
	ret.models = intersect(models, rhs.models);
	ret.fonts = intersect(fonts, rhs.fonts);
	return ret;
}

AssetList AssetList::operator-(const AssetList& rhs) const
{
	AssetList ret;
	ret.shaders = subtract(shaders, rhs.shaders);
	ret.textures = subtract(textures, rhs.textures);
	ret.cubemaps = subtract(cubemaps, rhs.cubemaps);
	ret.materials = subtract(materials, rhs.materials);
	ret.meshes = subtract(meshes, rhs.meshes);
	ret.models = subtract(models, rhs.models);
	ret.fonts = subtract(fonts, rhs.fonts);
	return ret;
}

} // namespace le
