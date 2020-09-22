#include <engine/game/scene.hpp>

namespace le
{
void GameScene::reset()
{
	Registry& reg = m_registry;
	reg.clear();
	m_name.clear();
	m_gameRect = {};
#if defined(LEVK_EDITOR)
	m_editorData = {};
#endif
}

GameScene::Desc& GameScene::desc()
{
	Registry& reg = m_registry;
	auto view = reg.view<Desc>();
	if (view.empty())
	{
		auto [_, desc] = reg.spawn<Desc>("scene_desc");
		auto& [desc_] = desc;
		return desc_;
	}
	else
	{
		auto [_, desc] = view.front();
		auto& [desc_] = desc;
		return desc_;
	}
}

gfx::Camera& GameScene::mainCamera()
{
	return desc().camera;
}

bool GameScene::reparent(Prop prop, Transform* pParent)
{
	if (prop.valid())
	{
		if constexpr (s_bParentToRoot)
		{
			if (!pParent)
			{
				pParent = &m_sceneRoot;
			}
		}
		prop.transform().parent(pParent);
		return true;
	}
	return false;
}

void GameScene::destroy(Span<Prop> props)
{
	Registry& reg = m_registry;
	for (auto& prop : props)
	{
		if (prop.pTransform)
		{
			m_entityMap.erase(*prop.pTransform);
		}
		reg.destroy(prop.entity);
	}
}

void GameScene::boltOnRoot(Prop prop)
{
	auto& transform = prop.transform();
	transform.parent(&m_sceneRoot);
	m_entityMap[transform] = prop.entity;
}
} // namespace le
