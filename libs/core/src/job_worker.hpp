#pragma once
#include <atomic>
#include "core/std_types.hpp"
#include "core/threads.hpp"

namespace le
{
class JobWorker final
{
public:
	enum class State : u8
	{
		eIdle,
		eBusy,
		eCOUNT_
	};

private:
	static std::atomic_bool s_bWork;

private:
	class JobManager* m_pManager;
	HThread m_hThread;
	std::atomic<State> m_state = State::eIdle;
	u8 id;

public:
	JobWorker(JobManager& manager, u8 id);
	~JobWorker();

private:
	void run();

	friend class JobManager;
};
} // namespace le
