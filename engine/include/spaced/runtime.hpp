#pragma once
#include <spaced/core/logger.hpp>
#include <spaced/engine.hpp>
#include <spaced/scene/scene_manager.hpp>

namespace spaced {
class Runtime {
  public:
	Runtime(Runtime const&) = delete;
	auto operator=(Runtime const&) -> Runtime& = delete;
	Runtime(Runtime&&) = delete;
	auto operator=(Runtime&&) -> Runtime& = delete;

	virtual ~Runtime() = default;

	explicit Runtime(std::string log_file_path = "spaced.log");

	auto build_engine(Engine::Builder builder) -> void;

	virtual auto run() -> void;

  protected:
	virtual auto setup() -> void;
	virtual auto tick(Duration dt) -> void;
	virtual auto render() const -> void;

	std::shared_ptr<logger::File> m_log_file{};
	std::unique_ptr<Engine> m_engine{};

	std::unique_ptr<SceneManager> m_scene_manager{};
};
} // namespace spaced
