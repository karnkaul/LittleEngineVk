#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <fmt/format.h>
#include <core/std_types.hpp>

namespace le::tasks
{
///
/// \brief typedef of a list of tasks
///
using List = std::vector<std::pair<std::function<void()>, std::string>>;

///
/// \brief Handle to a submitted task
///
class Handle final
{
public:
	///
	/// \brief Status of task
	///
	enum class Status : s8
	{
		eWaiting,
		eExecuting,
		eCompleted,
		eDiscarded,
		eError
	};

private:
	s64 const m_id;
	std::atomic<Status> m_status;
	std::string m_exception;

public:
	Handle(s64 id);

public:
	///
	/// \brief Obtain the status of this task
	///
	Status status() const;
	///
	/// \brief Check whether this task has been completed
	///
	bool hasCompleted(bool bIncludeDiscarded) const;
	s64 id() const;

	///
	/// \brief Wait until task is complete / discarded
	///
	void wait();
	///
	/// \brief Discard task if not already executing
	///
	bool discard();

	///
	/// \brief Check whether task threw an exception
	///
	bool didThrow() const;
	///
	/// \brief Obtain exception.what() (if Status is eError)
	///
	std::string_view exception() const;

private:
	friend class Worker;
};

///
/// \brief Enqueue a new task
///
std::shared_ptr<Handle> enqueue(std::function<void()> task, std::string name);
///
/// \brief Enqueue a list of tasks
///
std::vector<std::shared_ptr<Handle>> enqueue(List taskList);

///
/// \brief Enqueue a task per item in a container
///
template <typename T, template <typename, typename...> typename C, typename... Args>
std::vector<std::shared_ptr<Handle>> forEach(C<T, Args...>& out_itemList, std::function<void(T&)> task, std::string_view prefix)
{
	if (task)
	{
		List newTasks;
		newTasks.reserve(out_itemList.size());
		std::size_t idx = 0;
		for (typename C<T, Args...>::iterator iter = out_itemList.begin(); iter != out_itemList.end(); ++iter)
		{
			std::string name = prefix.empty() ? std::string() : fmt::format("{}-{}", prefix, idx++);
			auto newTask = [&out_itemList, iter, task]() mutable { task(*iter); };
			newTasks.push_back({std::move(newTask), std::move(name)});
		}
		return enqueue(std::move(newTasks));
	}
	return {};
}

///
/// \brief Wait for all tasks to complete
///
template <template <typename, typename...> typename C, typename... Args>
void wait(C<std::shared_ptr<Handle>, Args...>& out_handles)
{
	for (auto& handle : out_handles)
	{
		handle->wait();
	}
}

///
/// \brief Wait until no tasks are executing / enqueued
///
void waitIdle(bool bKillEnqueued);

///
/// \brief RAII Service to initialise/deinitialise tasks module
///
struct Service final
{
	Service(u8 workerCount = 2);
	~Service();
};

///
/// \brief Manually initialise tasks module
///
bool init(u8 workerCount);
///
/// \brief Manually deinitialise tasks module
///
void deinit();
} // namespace le::tasks
