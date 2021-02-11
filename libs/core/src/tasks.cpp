#include <algorithm>
#include <array>
#include <optional>
#include <vector>
#include <core/counter.hpp>
#include <core/log.hpp>
#include <core/tasks.hpp>
#include <core/threads.hpp>
#include <kt/async_queue/async_queue.hpp>

namespace le {
namespace tasks {
struct Worker final {
	threads::TScoped thread;

	Worker(std::size_t idx);
};

namespace {
struct Task final {
	std::function<void()> task;
	std::shared_ptr<Handle> handle;
	std::string name;
};

class Queue final {
  private:
	kt::async_queue<Task> m_queue;
	std::atomic<bool> m_bWork;
	TCounter<s64> m_nextID;

  public:
	Queue();
	~Queue();

  public:
	std::optional<Task> popTask();
	std::shared_ptr<Handle> pushTask(std::function<void()> task, std::string name);
	std::vector<std::shared_ptr<Handle>> pushTasks(List taskList);

	bool active() const;

	void clear();
	void waitIdle();

	void init();
	void deinit();
};

constexpr std::string_view g_tName = "tasks";
Queue g_queue;
std::vector<Worker> g_workers;
constexpr std::size_t maxWorkers = 256;
std::array<std::atomic<bool>, maxWorkers> g_busy;

Queue::Queue() {
	m_bWork.store(true);
}

Queue::~Queue() {
	ENSURE(m_queue.empty(), "Task Queue running past main!");
	deinit();
}

std::optional<Task> Queue::popTask() {
	return m_queue.pop();
}

std::shared_ptr<Handle> Queue::pushTask(std::function<void()> task, std::string name) {
	std::shared_ptr<Handle> ret;
	if (m_bWork.load()) {
		Task newTask;
		newTask.handle = std::make_shared<Handle>(++m_nextID);
		logD_if(!name.empty(), "[{}] task_{} [{}] enqueued", g_tName, newTask.handle->id(), name);
		newTask.task = std::move(task);
		newTask.name = std::move(name);
		ret = newTask.handle;
		m_queue.push(std::move(newTask));
	}
	return ret;
}

std::vector<std::shared_ptr<Handle>> Queue::pushTasks(List taskList) {
	std::vector<std::shared_ptr<Handle>> ret;
	if (m_bWork.load()) {
		std::vector<Task> newTasks;
		newTasks.reserve(taskList.size());
		ret.reserve(taskList.size());
		for (auto& task : taskList) {
			Task newTask;
			newTask.handle = std::make_shared<Handle>(++m_nextID);
			newTask.task = std::move(task.first);
			logD_if(!task.second.empty(), "[{}] task_{} [{}] enqueued", g_tName, newTask.handle->id(), task.second);
			newTask.name = std::move(task.second);
			ret.push_back(newTask.handle);
			newTasks.push_back(std::move(newTask));
		}
		m_queue.push(std::move(newTasks));
	}
	return ret;
}

bool Queue::active() const {
	return m_bWork.load();
}

void Queue::clear() {
	m_queue.clear(m_bWork.load());
}

void Queue::waitIdle() {
	while (!m_queue.empty()) {
		threads::sleep();
	}
}

void Queue::init() {
	m_bWork.store(true);
	clear();
}

void Queue::deinit() {
	m_bWork.store(false);
	clear();
}
} // namespace

Worker::Worker(std::size_t idx) {
	ENSURE(idx < g_busy.size(), "Invariant violated");
	thread = threads::newThread([idx]() {
		while (g_queue.active()) {
			g_busy[idx] = false;
			auto task = g_queue.popTask();
			if (task && task->handle && task->task) {
				auto const id = task->handle->id();
				if (task->handle->status() == Handle::Status::eDiscarded) {
					logI("[{}] task_{} [{}] discarded", g_tName, task->handle->id(), task->name);
					continue;
				}
				Handle::Status status = Handle::Status::eWaiting;
				if (task->handle->m_status.compare_exchange_strong(status, Handle::Status::eExecuting)) {
					g_busy[idx] = true;
					task->handle->m_status.store(Handle::Status::eExecuting);
					try {
						logD_if(!task->name.empty(), "[{}] starting task_{} [{}]...", g_tName, id, task->name);
						task->task();
						logD_if(!task->name.empty(), "[{}] task_{} [{}] completed", g_tName, id, task->name);
						task->handle->m_status.store(Handle::Status::eCompleted);
					} catch (std::exception const& e) {
						logE("[{}] task_{} [{}] threw an exception: {}", g_tName, id, task->name.empty() ? "Unnamed" : task->name, e.what());
						task->handle->m_exception = e.what();
						task->handle->m_status.store(Handle::Status::eError);
					}
				}
			}
		}
	});
}

Handle::Handle(s64 id) : m_id(id), m_status(Status::eWaiting) {
}

Handle::Status Handle::status() const noexcept {
	return m_status.load();
}

bool Handle::hasCompleted(bool bIncludeDiscarded) const noexcept {
	auto const status = m_status.load();
	return status == Status::eCompleted || status == Status::eError || (bIncludeDiscarded && status == Status::eDiscarded);
}

s64 Handle::id() const noexcept {
	return m_id;
}

void Handle::wait() {
	while (!hasCompleted(true)) {
		threads::sleep();
	}
}

bool Handle::discard() noexcept {
	if (m_status.load() != Status::eExecuting) {
		m_status.store(Status::eDiscarded);
		return true;
	}
	return false;
}

bool Handle::didThrow() const noexcept {
	return m_status.load() == Status::eError;
}

std::string_view Handle::exception() const noexcept {
	return m_exception;
}

Service::Service(u8 workerCount) {
	if (!init(workerCount)) {
		logE("[{}] Failed to initialise task workers!", g_tName);
	}
}

Service::~Service() {
	deinit();
}
} // namespace tasks

std::shared_ptr<tasks::Handle> tasks::enqueue(std::function<void()> task, std::string name) {
	return g_queue.pushTask(std::move(task), std::move(name));
}

std::vector<std::shared_ptr<tasks::Handle>> tasks::enqueue(List taskList) {
	return g_queue.pushTasks(std::move(taskList));
}

void tasks::waitIdle(bool bKillEnqueued) {
	if (bKillEnqueued) {
		g_queue.clear();
	} else {
		g_queue.waitIdle();
	}
	while (std::any_of(g_busy.begin(), g_busy.end(), [](auto const& busy) -> bool { return busy; })) {
		threads::sleep();
	}
}

bool tasks::init(u8 workerCount) {
	if (g_workers.empty() && workerCount > 0) {
		g_queue.init();
		for (u8 count = 0; count < workerCount; ++count) {
			g_workers.push_back(Worker((std::size_t)count));
		}
		return true;
	}
	return false;
}

void tasks::deinit() {
	waitIdle(true);
	g_queue.deinit();
	g_workers.clear();
}
} // namespace le
