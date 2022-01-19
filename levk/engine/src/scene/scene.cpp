#include <levk/core/services.hpp>
#include <levk/engine/scene/scene.hpp>

namespace le {
Scene::Scene(Opt<Engine::Service> service) noexcept : m_engineService(service ? *service : *Services::find<Engine::Service>()) {}

dts::scheduler& Scene::scheduler() const {
	EXPECT(m_taskScheduler);
	return *m_taskScheduler;
}

void Scene::tick(Time_s dt) {
	updateSystems(scheduler(), dt, engine());
	scheduler().rethrow();
}

void Scene::close() { scheduler().clear(); }
} // namespace le
