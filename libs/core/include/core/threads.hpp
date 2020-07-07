#pragma once
#include <functional>
#include <thread>
#include <core/std_types.hpp>
#include <core/time.hpp>
#include <core/zero.hpp>

namespace le
{
namespace threads
{
///
/// \brief Handle for a running thread
///
using Handle = TZero<u64>;

///
/// \brief Initialise threads
///
void init();
///
/// \brief Create a new thread
///
Handle newThread(std::function<void()> task);
///
/// \brief Join a thread
///
void join(Handle& id);
///
/// \brief Join all running threads
///
void joinAll();

///
/// \brief Obtain the handle for this thread
///
Handle thisThreadID();
///
/// \brief Check whether this is the main thread (which called `init()`)
///
bool isMainThread();
///
/// \brief Obtain max supported threads in hardware
///
u32 maxHardwareThreads();
///
/// \brief Obtain the number of running threads
///
u32 runningCount();

///
/// \brief Sleep this thread
/// \param duration pass zero to yield
void sleep(Time duration = {});

///
/// \brief Yield this thread until `f` returns `true`
///
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
