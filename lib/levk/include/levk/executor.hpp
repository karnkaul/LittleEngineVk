#pragma once
#include <levk/core/polymorphic.hpp>
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace levk {
enum struct ExecStage : int { eDefault = 0 };

using ExecFunc = std::move_only_function<void()>;

template <std::same_as<ExecStage> S0, std::same_as<ExecStage> S1, std::same_as<ExecStage>... Sn>
constexpr auto max_exec_stage(S0 s0, S1 s1, Sn... sn) {
	if constexpr (sizeof...(Sn) == 0) {
		return std::max(s0, s1);
	} else {
		return std::max(s0, max_exec_stage(s1, sn...));
	}
}

constexpr auto next_stage(ExecStage const in) { return static_cast<ExecStage>(static_cast<int>(in) + 1); }

template <std::same_as<ExecStage> Dep, std::same_as<ExecStage>... Deps>
constexpr auto dependent_exec_stage(Dep dep, Deps... deps) {
	if constexpr (sizeof...(Deps) == 0) {
		return next_stage(dep);
	} else {
		return next_stage(max_exec_stage(dep, deps...));
	}
}

struct ExecTask {
	ExecStage stage{};
	ExecFunc func{};
};

struct ExecStatus {
	std::int64_t total{};
	std::int64_t completed{};

	[[nodiscard]] constexpr auto get_pending() const -> std::int64_t { return total - completed; }
	[[nodiscard]] constexpr auto get_progress() const -> float { return total == 0 ? -1.0f : static_cast<float>(completed) / static_cast<float>(total); }

	[[nodiscard]] constexpr auto is_busy() const -> bool { return completed < total; }
};

class IExecutor : public Polymorphic {
  public:
	virtual auto update() -> ExecStatus = 0;
	virtual void kill_pending() = 0;
};

[[nodiscard]] auto create_executor(std::vector<ExecTask> tasks, std::optional<int> workers = {}) -> std::unique_ptr<IExecutor>;
} // namespace levk
