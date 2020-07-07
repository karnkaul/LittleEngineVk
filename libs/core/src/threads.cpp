#include <utility>
#include <list>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/os.hpp>
#include <core/threads.hpp>
#if defined(LEVK_OS_LINUX)
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#endif

namespace le
{
namespace
{
threads::Handle g_nextID = threads::Handle::s_null;
std::list<std::pair<threads::Handle::type, std::thread>> g_threads;
std::unordered_map<std::thread::id, threads::Handle> g_idMap;
std::thread::id g_mainThreadID;
} // namespace

void threads::init()
{
	g_mainThreadID = std::this_thread::get_id();
#if defined(LEVK_OS_LINUX)
	if (XInitThreads() == 0)
	{
		LOG_E("[OS] ERROR calling XInitThreads()! UB follows.");
	}
#endif
	return;
}

threads::Handle threads::newThread(std::function<void()> task)
{
	g_threads.emplace_back(++g_nextID.handle, std::thread(task));
	g_idMap[g_threads.back().second.get_id()] = g_nextID;
	return Handle(g_nextID);
}

void threads::join(Handle& id)
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
	id = Handle();
	return;
}

void threads::joinAll()
{
	for (auto& [id, thread] : g_threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
	g_idMap.clear();
	g_threads.clear();
	return;
}

threads::Handle threads::thisThreadID()
{
	auto search = g_idMap.find(std::this_thread::get_id());
	return search != g_idMap.end() ? search->second : Handle();
}

bool threads::isMainThread()
{
	return std::this_thread::get_id() == g_mainThreadID;
}

u32 threads::maxHardwareThreads()
{
	return std::thread::hardware_concurrency();
}

u32 threads::runningCount()
{
	return (u32)g_threads.size();
}

void threads::sleep(Time duration)
{
	if (duration <= Time(0))
	{
		std::this_thread::yield();
	}
	else
	{
		std::this_thread::sleep_for(std::chrono::microseconds(duration.to_us()));
	}
}
} // namespace le
