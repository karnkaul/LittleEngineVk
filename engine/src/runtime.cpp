#include <spaced/runtime.hpp>

namespace spaced {
Runtime::Runtime(std::string log_file_path) : m_log_file(logger::log_to_file(std::move(log_file_path))) {}

auto Runtime::build_engine(Engine::Builder builder) -> void {
	if (m_engine || Engine::exists()) { return; }
	m_engine = builder.build();
}

auto Runtime::run() -> void {
	if (!m_engine) { m_engine = Engine::Builder{"Spaced"}.build(); }

	setup();

	auto delta_time = DeltaTime{};
	while (m_engine->next_frame()) {
		tick(delta_time());
		render();
	}
}

auto Runtime::tick(Duration dt) -> void { m_scene_manager.tick(dt); }

auto Runtime::render() const -> void { m_scene_manager.render(); }
} // namespace spaced
