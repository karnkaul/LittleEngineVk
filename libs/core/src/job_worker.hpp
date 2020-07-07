#pragma once
#include <atomic>
#include <core/std_types.hpp>
#include <core/threads.hpp>

namespace le::jobs
{
class Worker final
{
public:
	enum class State : s8
	{
		eIdle,
		eBusy,
		eCOUNT_
	};

private:
	static std::atomic_bool s_bWork;

private:
	class Manager* m_pManager;
	threads::Handle m_hThread;
	std::atomic<State> m_state = State::eIdle;
	u8 id;

public:
	Worker(Manager& manager, u8 id);
	~Worker();

private:
	void run();

	friend class Manager;
};
} // namespace le::jobs
