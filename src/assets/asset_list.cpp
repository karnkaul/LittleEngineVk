#include <algorithm>
#include <engine/assets/asset_list.hpp>

namespace le
{
namespace
{
std::vector<stdfs::path> intersect(std::vector<stdfs::path> const& lhs, std::vector<stdfs::path> const& rhs)
{
	std::vector<stdfs::path> ret;
	for (auto& asset : lhs)
	{
		auto const search = std::find_if(rhs.begin(), rhs.end(), [&asset](stdfs::path const& rhs) -> bool { return asset == rhs; });
		if (search != rhs.end())
		{
			ret.push_back(asset);
		}
	}
	return ret;
}

void subtract(std::vector<stdfs::path> const& lhs, std::vector<stdfs::path>& out_rhs)
{
	for (auto& asset : lhs)
	{
		auto iter = std::remove_if(out_rhs.begin(), out_rhs.end(), [&asset](stdfs::path const& rhs) -> bool { return asset == rhs; });
		out_rhs.erase(iter, out_rhs.end());
	}
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
	AssetList ret = rhs;
	subtract(shaders, ret.shaders);
	subtract(textures, ret.textures);
	subtract(cubemaps, ret.cubemaps);
	subtract(materials, ret.materials);
	subtract(meshes, ret.meshes);
	subtract(models, ret.models);
	subtract(fonts, ret.fonts);
	return ret;
}

} // namespace le
