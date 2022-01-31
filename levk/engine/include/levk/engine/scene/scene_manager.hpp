#pragma once
#include <levk/engine/editor/types.hpp>
#include <levk/engine/render/shader_buffer_map.hpp>
#include <levk/engine/scene/scene.hpp>

namespace le {
class SceneManager {
  public:
	SceneManager(Engine::Service engine, Opt<Editor> editor = {});
	SceneManager(SceneManager&&) = default;
	SceneManager& operator=(SceneManager&&) = default;
	~SceneManager();

	template <typename T, typename... Args>
		requires(std::derived_from<T, Scene>)
	T& attach(std::string id, Args&&... args);

	Viewport const& sceneView() const noexcept;
	Opt<Scene> active() const noexcept { return m_active ? m_active->scene.get() : nullptr; }
	bool open(Hash id);
	void tick(Time_s dt);
	void render(graphics::RGBA clear = {});
	void close();

	editor::SceneRef sceneRef() const noexcept { return m_active ? m_active->scene->ediScene() : editor::SceneRef(); }

  private:
	struct Entry {
		std::string id;
		std::unique_ptr<Scene> scene;
	};
	std::unordered_map<Hash, Entry> m_scenes;
	ShaderBufferMap m_shaderBufferMap;
	Engine::Service m_engine;
	Opt<Editor> m_editor{};
	Opt<Entry> m_active{};
};

// impl

template <typename T, typename... Args>
	requires(std::derived_from<T, Scene>)
T& SceneManager::attach(std::string id, Args&&... args) {
	auto t = std::make_unique<T>(std::forward<Args>(args)...);
	auto& ret = *t;
	ret.m_engineService = m_engine;
	ret.m_shaderBufferMap = &m_shaderBufferMap;
	Hash const hash = id;
	m_scenes.insert_or_assign(hash, Entry{std::move(id), std::move(t)});
	return ret;
}
} // namespace le
