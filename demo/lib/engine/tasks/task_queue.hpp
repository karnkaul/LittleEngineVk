#pragma once
#include <atomic>
#include <functional>
#include <shared_mutex>
#include <unordered_map>
#include <core/ref.hpp>
#include <core/std_types.hpp>
#include <core/threads.hpp>
#include <kt/async_queue/async_queue.hpp>

namespace le {
struct TaskID {
	using type = u64;

	type id = 0;
};

class TaskQueue {
  public:
	enum class Status {
		eEnqueued,
		eExecuting,
		eDone,
		eError,
		eUnknown,
	};

	using Task = std::function<void()>;

	TaskQueue(u8 workerCount = 4, bool bCatchRuntimeErrors = levk_debug);
	virtual ~TaskQueue();

	TaskID enqueue(Task const& task);

	Status status(TaskID id) const;
	bool wait(TaskID id);

  protected:
	template <typename K, typename V, V End>
	struct StatMap {
		std::unordered_map<K, V> map;
		mutable kt::lockable<std::shared_mutex> mutex;

		void set(K key, V value);
		V get(K key) const;
		bool wait(K key);
	};

  private:
	using Entry = std::pair<TaskID, Task>;
	using StatusMap = StatMap<TaskID::type, Status, Status::eDone>;

	struct Worker {
		inline static bool bCatch = true;

		threads::TScoped thread;

		Worker(StatusMap& status, kt::async_queue<Entry>& queue, u8 id);

		static void run(StatusMap& status, Entry const& entry);
	};

	StatusMap m_status;
	std::vector<Worker> m_workers;
	std::atomic<TaskID::type> m_nextID;
	kt::async_queue<Entry> m_queue;
};

// impl

template <typename K, typename V, V End>
void TaskQueue::StatMap<K, V, End>::set(K key, V value) {
	auto lock = mutex.lock<std::unique_lock>();
	if (value == End) {
		map.erase(key);
	} else {
		map[key] = value;
	}
}
template <typename K, typename V, V End>
V TaskQueue::StatMap<K, V, End>::get(K key) const {
	auto lock = mutex.lock<std::shared_lock>();
	if (auto it = map.find(key); it != map.end()) {
		return it->second;
	}
	return End;
}
template <typename K, typename V, V End>
bool TaskQueue::StatMap<K, V, End>::wait(K key) {
	auto lock = mutex.lock<std::unique_lock>();
	auto it = map.find(key);
	while (it != map.end() && it->second < End) {
		lock.unlock();
		threads::sleep();
		lock.lock();
		it = map.find(key);
	}
	if (it != map.end() && it->second == End) {
		map.erase(it);
		it = map.end();
	}
	return it == map.end();
}
} // namespace le
