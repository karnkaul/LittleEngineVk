#include <levk/executor.hpp>

namespace levk {
namespace {
class Executor : public IExecutor {
  public:
	explicit Executor(std::vector<ExecTask> tasks, int const workers) {
		if (tasks.empty()) { return; }

		for (auto& task : tasks) {
			m_map[task.stage].push_back(std::move(task.func));
			++m_total;
		}
		refresh_queue();
		start(workers);
	}

  private:
	struct Worker {
		std::jthread thread{};
		std::atomic<bool> is_busy{};
	};

	auto update() -> ExecStatus final {
		refresh_queue();
		return ExecStatus{.total = m_total, .completed = m_completed};
	}

	void kill_pending() final {
		auto lock = std::scoped_lock{m_mutex};
		m_map.clear();
		m_queue.clear();
		m_completed = 0;
		m_total = 0;
	}

	void start(int workers) {
		assert(workers > 0);
		m_workers.reserve(static_cast<std::size_t>(workers));
		for (; workers > 0; --workers) {
			auto worker = std::make_unique<Worker>();
			worker->thread = std::jthread{[this, busy = &worker->is_busy](std::stop_token const& s) { run_worker(s, *busy); }};
			m_workers.push_back(std::move(worker));
		}
	}

	void refresh_queue() {
		if (m_map.empty()) { return; }

		auto lock = std::unique_lock{m_mutex};
		if (!m_queue.empty()) { return; }

		if (!std::ranges::all_of(m_workers, [](auto const& w) { return !w->is_busy; })) { return; }

		auto const it = m_map.begin();
		for (auto& func : it->second) { m_queue.push_back(std::move(func)); }
		m_map.erase(it);
		lock.unlock();
		m_cv.notify_all();
	}

	void run_worker(std::stop_token const& stop, std::atomic<bool>& busy) {
		auto func = ExecFunc{};
		while (pop_func(func, stop)) {
			busy = true;
			func();
			++m_completed;
			busy = false;
		}
	}

	auto pop_func(ExecFunc& out, std::stop_token const& stop) -> bool {
		auto lock = std::unique_lock{m_mutex};
		auto const pred = [this] { return !m_queue.empty(); };
		if (!m_cv.wait(lock, stop, pred)) { return false; }
		assert(!m_queue.empty());
		out = std::move(m_queue.front());
		m_queue.pop_front();
		return true;
	}

	std::int64_t m_total{};
	std::mutex m_mutex{};
	std::condition_variable_any m_cv{};
	std::vector<std::unique_ptr<Worker>> m_workers{};

	std::map<ExecStage, std::vector<ExecFunc>> m_map{};
	std::deque<ExecFunc> m_queue{};
	std::atomic<std::int64_t> m_completed{};
};
} // namespace
} // namespace levk

auto levk::create_executor(std::vector<ExecTask> tasks, std::optional<int> workers) -> std::unique_ptr<IExecutor> {
	static auto const max_threads = static_cast<int>(std::thread::hardware_concurrency());
	workers = std::clamp(workers.value_or(max_threads), 1, max_threads);
	return std::make_unique<Executor>(std::move(tasks), *workers);
}
