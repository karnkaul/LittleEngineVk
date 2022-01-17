#pragma once
#include <levk/engine/editor/types.hpp>
#include <levk/engine/scene/scene.hpp>

namespace le {
class SceneManager {
  public:
	SceneManager(Engine::Service engine) : m_engine(engine), m_scheduler(std::make_unique<dts::scheduler>()) {}
	SceneManager(SceneManager&&) = default;
	SceneManager& operator=(SceneManager&&) = default;
	~SceneManager();

	template <typename T, typename... Args>
		requires(std::is_base_of_v<Scene, T>)
	T& attach(std::string id, Args&&... args);

	Opt<Scene> active() const noexcept { return m_active ? m_active->scene.get() : nullptr; }
	bool open(Hash id);
	void tick(Time_s dt);
	void render(graphics::RGBA clear = {});
	void close();

	edi::SceneRef sceneRef() const noexcept { return m_active ? m_active->scene->ediScene() : edi::SceneRef(); }

  private:
	struct Entry {
		std::string id;
		std::unique_ptr<Scene> scene;
	};
	std::unordered_map<Hash, Entry> m_scenes;
	Engine::Service m_engine;
	std::unique_ptr<dts::scheduler> m_scheduler;
	Opt<Entry> m_active{};
};

// impl

template <typename T, typename... Args>
	requires(std::is_base_of_v<Scene, T>)
T& SceneManager::attach(std::string id, Args&&... args) {
	auto t = std::make_unique<T>(std::forward<Args>(args)...);
	auto& ret = *t;
	ret.m_engineService = m_engine;
	ret.m_taskScheduler = m_scheduler.get();
	Hash const hash = id;
	m_scenes.insert_or_assign(hash, Entry{std::move(id), std::move(t)});
	return ret;
}
} // namespace le
