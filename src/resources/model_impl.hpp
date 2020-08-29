#pragma once
#include <engine/resources/resources.hpp>
#include <resources/resources_impl.hpp>

namespace le::res
{
class Model::Impl : public ImplBase, ILoadable
{
public:
	std::deque<res::Material::Inst> m_materials;
	std::vector<res::Mesh> m_meshes;
	std::deque<res::Scoped<res::Mesh>> m_loadedMeshes;
	std::unordered_map<Hash, res::Scoped<res::Material>> m_loadedMaterials;
	std::unordered_map<Hash, res::Scoped<res::Texture>> m_textures;

	std::vector<res::Mesh> meshes() const;
#if defined(LEVK_EDITOR)
	std::deque<res::Scoped<res::Mesh>>& loadedMeshes();
#endif

	bool make(CreateInfo& out_createInfo, Info& out_info);
	void release();

	bool update();
};

Model::Impl* impl(Model model);
Model::Info* infoRW(Model model);
} // namespace le::res
