#include <thread>
#include <utility>
#include <list>
#include "core/assert.hpp"
#include "core/threads.hpp"
#include "threads_impl.hpp"

namespace le
{
namespace
{
HThread::Type g_nextID = HThread::Null;
std::list<std::pair<HThread::Type, std::thread>> g_threads;
std::unordered_map<std::thread::id, HThread::Type> g_idMap;
} // namespace

using namespace threadsImpl;

HThread threads::newThread(std::function<void()> task)
{
	g_threads.emplace_back(++g_nextID, std::thread(task));
	g_idMap[g_threads.back().second.get_id()] = g_nextID;
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
		g_idMap.erase(thread.get_id());
		g_threads.erase(search);
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
	g_idMap.clear();
	g_threads.clear();
	return;
}

HThread::Type threads::thisThreadID()
{
	auto search = g_idMap.find(std::this_thread::get_id());
	return search != g_idMap.end() ? search->second : HThread::Null;
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
