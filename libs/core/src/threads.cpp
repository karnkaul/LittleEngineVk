#include <thread>
#include <utility>
#include <vector>
#include "core/assert.hpp"
#include "core/threads.hpp"
#include "threads_impl.hpp"

namespace le
{
namespace
{
s32 g_nextID = 0;
std::vector<std::pair<s32, std::thread>> g_threads;
} // namespace

using namespace threadsImpl;

HThread threads::newThread(std::function<void()> task)
{
	g_threads.emplace_back(++g_nextID, std::thread(task));
	return HThread(g_nextID);
}

void threads::join(HThread& id)
{
	auto search = std::find_if(g_threads.begin(), g_threads.end(), [id](auto const& t) -> bool { return t.first == id; });
	if (search != g_threads.end())
	{
		auto& thread = search->second;
		if (thread.joinable())
		{
			thread.join();
		}
	}
	id = HThread();
	return;
}

void threads::joinAll()
{
	for (auto& kvp : g_threads)
	{
		auto& thread = kvp.second;
		if (thread.joinable())
		{
			thread.join();
		}
	}
	g_threads.clear();
	return;
}

u32 threads::maxHardwareThreads()
{
	return g_maxThreads;
}

u32 threads::running()
{
	return (u32)g_threads.size();
}
} // namespace le
