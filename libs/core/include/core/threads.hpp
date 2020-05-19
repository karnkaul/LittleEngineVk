#pragma once
#include <functional>
#include <thread>
#include "std_types.hpp"
#include "time.hpp"
#include "zero.hpp"

namespace le
{
using HThread = TZero<u64>;

namespace threads
{
void init();
HThread newThread(std::function<void()> task);
void join(HThread& id);
void joinAll();

HThread::Type thisThreadID();
bool isMainThread();
u32 maxHardwareThreads();
u32 runningCount();

void sleep(Time duration = {});

template <typename F>
void sleepUntil(F f)
{
	while (!f())
	{
		sleep();
	}
}
} // namespace threads
} // namespace le
