#include <dumb_tasks/executor.hpp>
#include <levk/core/services.hpp>
#include <levk/engine/render/shader_buffer_map.hpp>
#include <levk/gameplay/scene/scene.hpp>
#include <levk/graphics/render/context.hpp>

namespace le {
Scene::Scene(Opt<Engine::Service> service) noexcept : m_engineService(service ? *service : *Services::find<Engine::Service>()) {}

ShaderBufferMap& Scene::shaderBufferMap() const {
	EXPECT(m_shaderBufferMap);
	return *m_shaderBufferMap;
}

graphics::ShaderBuffer& Scene::shaderBuffer(Hash id) const {
	EXPECT(m_shaderBufferMap);
	return m_shaderBufferMap->get(id);
}

void Scene::open() { executor().start(); }

void Scene::tick(Time_s dt) { updateSystems(dt, engine()); }

void Scene::close() {
	executor().stop();
	engine().context().pipelineFactory().clear();
}
} // namespace le
