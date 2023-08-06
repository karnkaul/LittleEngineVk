#include <le/runtime.hpp>

namespace le {
Runtime::Runtime(std::string log_file_path) : m_log_file(logger::log_to_file(std::move(log_file_path))) {}

auto Runtime::build_engine(Engine::Builder builder) -> void {
	if (m_engine || Engine::exists()) { return; }
	m_engine = builder.build();
}

auto Runtime::run() -> void {
	if (!m_engine) { m_engine = Engine::Builder{"little-engine"}.build(); }

	setup();

	while (m_engine->next_frame()) {
		tick(m_engine->delta_time());
		render();
	}
}

auto Runtime::setup() -> void { m_scene_manager = std::make_unique<SceneManager>(); }

auto Runtime::tick(Duration dt) -> void { m_scene_manager->tick(dt); }

auto Runtime::render() const -> void { m_scene_manager->render(); }
} // namespace le
