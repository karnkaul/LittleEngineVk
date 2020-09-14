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
/// \brief ID of a running thread
///
using ID = TZero<u64>;

///
/// \brief RAII wrapper for a thread instance (joins in destructor)
///
struct Scoped final : NoCopy
{
public:
	constexpr Scoped(ID id = ID::null) noexcept : id_(id) {}
	constexpr Scoped(Scoped&&) noexcept = default;
	Scoped& operator=(Scoped&&);
	~Scoped();

	///
	/// \brief Obtain the ID of the thread of execution this handle points to
	///
	ID id() const noexcept;
	///
	/// \brief Check if pointing to instance of thread execution this handle points to
	///
	bool valid() const noexcept;
	///
	/// \brief Block calling thread until instance of thread execution handle points to joins
	///
	void join();

private:
	ID id_;
};

///
/// \brief Initialise threads
///
void init();
///
/// \brief Create a new thread
///
Scoped newThread(std::function<void()> task);
///
/// \brief Join a thread
///
void join(ID& out_id);
///
/// \brief Join all running threads
///
void joinAll();

///
/// \brief Obtain the ID for this thread
///
ID thisThreadID();
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
