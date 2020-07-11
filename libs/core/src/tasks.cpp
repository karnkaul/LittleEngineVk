#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <core/threads.hpp>
#include <core/log.hpp>
#include <core/tasks.hpp>
#include <core/utils.hpp>

namespace le
{
namespace tasks
{
class Worker final
{
private:
	threads::Handle m_thread;
	std::atomic<bool> m_bBusy;

public:
	Worker();
	~Worker();

public:
	bool isBusy() const;

private:
	void run();
};

namespace
{
struct Task final
{
	std::function<void()> task;
	std::shared_ptr<Handle> handle;
	std::string name;
};

class Queue final
{
private:
	std::deque<Task> m_queue;
	Lockable<std::mutex> m_mutex;
	std::condition_variable m_cv;
	std::atomic<bool> m_bWork;
	std::atomic<s64> m_nextID;

public:
	Queue();
	~Queue();

public:
	Task popTask();
	std::shared_ptr<Handle> pushTask(std::function<void()> task, std::string name);
	std::vector<std::shared_ptr<Handle>> pushTasks(List taskList);

	bool isActive() const;

	void clear();
	void waitIdle();

	void init();
	void deinit();
};

std::string_view const g_tName = "le::tasks";
Queue g_queue;
std::vector<std::unique_ptr<Worker>> g_workers;

Queue::Queue()
{
	m_bWork.store(true);
	m_nextID.store(0);
}

Queue::~Queue()
{
	ASSERT(m_queue.empty(), "Task Queue running past main!");
	deinit();
}

Task Queue::popTask()
{
	auto lock = m_mutex.lock<std::unique_lock>();
	if (m_queue.empty())
	{
		m_cv.wait(lock, [this]() { return !m_bWork.load() || !m_queue.empty(); });
	}
	if (m_bWork.load())
	{
		if (!m_queue.empty())
		{
			auto task = std::move(m_queue.front());
			m_queue.pop_front();
			bool const bNotify = !m_queue.empty();
			lock.unlock();
			if (bNotify)
			{
				m_cv.notify_one();
			}
			if (task.handle)
			{
				switch (task.handle->status())
				{
				case Handle::Status::eWaiting:
				{
					return task;
				}
				case Handle::Status::eDiscarded:
				{
					LOG_I("[{}] task_{} [{}] discarded", g_tName, task.handle->id(), task.name);
					break;
				}
				default:
					break;
				}
			}
		}
	}
	return {};
}

std::shared_ptr<Handle> Queue::pushTask(std::function<void()> task, std::string name)
{
	std::shared_ptr<Handle> ret;
	if (m_bWork.load())
	{
		Task newTask;
		newTask.handle = std::make_shared<Handle>(++m_nextID);
		LOGIF_D(!name.empty(), "[{}] task_{} [{}] enqueued", g_tName, newTask.handle->id(), name);
		newTask.task = std::move(task);
		newTask.name = std::move(name);
		ret = newTask.handle;
		{
			auto lock = m_mutex.lock();
			m_queue.push_back(std::move(newTask));
		}
		m_cv.notify_one();
	}
	return ret;
}

std::vector<std::shared_ptr<Handle>> Queue::pushTasks(List taskList)
{
	std::vector<std::shared_ptr<Handle>> ret;
	if (m_bWork.load())
	{
		std::vector<Task> newTasks;
		newTasks.reserve(taskList.size());
		ret.reserve(taskList.size());
		for (auto& task : taskList)
		{
			Task newTask;
			newTask.handle = std::make_shared<Handle>(++m_nextID);
			newTask.task = std::move(task.first);
			LOGIF_D(!task.second.empty(), "[{}] task_{} [{}] enqueued", g_tName, newTask.handle->id(), task.second);
			newTask.name = std::move(task.second);
			ret.push_back(newTask.handle);
			newTasks.push_back(std::move(newTask));
		}
		{
			auto lock = m_mutex.lock();
			std::move(newTasks.begin(), newTasks.end(), std::back_inserter(m_queue));
		}
		m_cv.notify_all();
	}
	return ret;
}

bool Queue::isActive() const
{
	return m_bWork.load();
}

void Queue::clear()
{
	auto lock = m_mutex.lock();
	m_queue.clear();
}

void Queue::waitIdle()
{
	bool bIdle = false;
	do
	{
		auto lock = m_mutex.lock();
		bIdle = m_queue.empty();
	} while (!bIdle);
}

void Queue::init()
{
	m_bWork.store(true);
}

void Queue::deinit()
{
	m_bWork.store(false);
	clear();
	m_cv.notify_all();
}
} // namespace

Worker::Worker()
{
	m_thread = threads::newThread([this]() { run(); });
	m_bBusy.store(false);
}

Worker::~Worker()
{
	threads::join(m_thread);
}

void Worker::run()
{
	while (g_queue.isActive())
	{
		m_bBusy.store(false);
		auto task = g_queue.popTask();
		if (task.handle && task.task)
		{
			auto const id = task.handle->id();
			Handle::Status status = Handle::Status::eWaiting;
			if (task.handle->m_status.compare_exchange_strong(status, Handle::Status::eExecuting))
			{
				task.handle->m_status.store(Handle::Status::eExecuting);
				m_bBusy.store(true);
				try
				{
					LOGIF_D(!task.name.empty(), "[{}] starting task_{} [{}]...", g_tName, id, task.name);
					task.task();
					LOGIF_D(!task.name.empty(), "[{}] task_{} [{}] completed", g_tName, id, task.name);
					task.handle->m_status.store(Handle::Status::eCompleted);
				}
				catch (std::exception const& e)
				{
					LOG_E("[{}] task_{} [{}] threw an exception: {}", g_tName, id, task.name.empty() ? "Unnamed" : task.name, e.what());
					task.handle->m_exception = e.what();
					task.handle->m_status.store(Handle::Status::eError);
				}
			}
		}
	}
}

bool Worker::isBusy() const
{
	return m_bBusy.load();
}

Handle::Handle(s64 id) : m_id(id), m_status(Status::eWaiting) {}

Handle::Status Handle::status() const
{
	return m_status.load();
}

bool Handle::hasCompleted(bool bIncludeDiscarded) const
{
	auto const status = m_status.load();
	return status == Status::eCompleted || status == Status::eError || (bIncludeDiscarded && status == Status::eDiscarded);
}

s64 Handle::id() const
{
	return m_id;
}

void Handle::wait()
{
	while (!hasCompleted(true))
	{
		threads::sleep();
	}
}

bool Handle::discard()
{
	if (m_status.load() != Status::eExecuting)
	{
		m_status.store(Status::eDiscarded);
		return true;
	}
	return false;
}

bool Handle::didThrow() const
{
	return m_status.load() == Status::eError;
}

std::string_view Handle::exception() const
{
	return m_exception;
}

Service::Service(u8 workerCount)
{
	if (!init(workerCount))
	{
		LOG_E("[{}] Failed to initialise task workers!", g_tName);
	}
}

Service::~Service()
{
	deinit();
}
} // namespace tasks

std::shared_ptr<tasks::Handle> tasks::enqueue(std::function<void()> task, std::string name)
{
	return g_queue.pushTask(std::move(task), std::move(name));
}

std::vector<std::shared_ptr<tasks::Handle>> tasks::enqueue(List taskList)
{
	return g_queue.pushTasks(std::move(taskList));
}

void tasks::waitIdle(bool bKillEnqueued)
{
	if (bKillEnqueued)
	{
		g_queue.clear();
	}
	else
	{
		g_queue.waitIdle();
	}
	while (std::any_of(g_workers.begin(), g_workers.end(), [](auto const& worker) -> bool { return worker->isBusy(); }))
	{
		threads::sleep();
	}
}

bool tasks::init(u8 workerCount)
{
	if (g_workers.empty() && workerCount > 0)
	{
		g_queue.init();
		g_workers.reserve((std::size_t)workerCount);
		for (u8 count = 0; count < workerCount; ++count)
		{
			g_workers.push_back(std::make_unique<Worker>());
		}
		return true;
	}
	return false;
}

void tasks::deinit()
{
	waitIdle(true);
	g_queue.deinit();
	g_workers.clear();
}
} // namespace le
