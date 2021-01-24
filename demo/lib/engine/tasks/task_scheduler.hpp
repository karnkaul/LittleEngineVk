#pragma once
#include <list>
#include <engine/tasks/task_queue.hpp>

namespace le {
class TaskScheduler : public TaskQueue {
  public:
	using TaskQueue::Status;
	using TaskQueue::Task;

	struct StageID {
		using type = u64;

		type id = 0;
	};

	struct Stage {
		std::string name;
		std::vector<StageID> deps;
		std::vector<Task> tasks;
	};

	TaskScheduler(u8 workerCount = 4, bool bCatchRuntimeErrors = levk_debug);
	~TaskScheduler();

	StageID stage(Stage&& stage);
	StageID stage(Stage const& stage);

	Status stageStatus(StageID id) const;
	bool wait(StageID id);

  private:
	struct Entry {
		std::vector<Task> tasks;
		std::vector<TaskID> ids;
		std::vector<StageID> deps;
		std::string stage;
		StageID id;
	};

	using StageStat = StatMap<StageID::type, Status, Status::eDone>;

	StageStat m_stageStat;
	std::atomic<StageID::type> m_nextID;
	std::list<Entry> m_waiting;
	std::list<Entry> m_running;
	mutable kt::lockable<> m_mutex;
	threads::TScoped m_thread;
	std::atomic<bool> m_bWork;
};
} // namespace le
