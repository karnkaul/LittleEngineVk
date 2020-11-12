#include <list>
#include <utility>
#include <core/ensure.hpp>
#include <core/log.hpp>
#include <core/os.hpp>
#include <core/threads.hpp>
#if defined(LEVK_OS_LINUX)
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#endif

namespace le {
namespace {
threads::ID g_nextID = threads::ID::null;
std::list<std::pair<threads::ID::type, std::thread>> g_threads;
std::unordered_map<std::thread::id, threads::ID> g_idMap;
std::thread::id g_mainThreadID;
} // namespace

threads::TScoped& threads::TScoped::operator=(TScoped&& rhs) {
	if (&rhs != this) {
		join();
		id_ = std::move(rhs.id_);
	}
	return *this;
}

threads::TScoped::~TScoped() {
	join();
}

bool threads::TScoped::valid() const noexcept {
	return id_.payload != ID::null;
}

threads::ID threads::TScoped::id() const noexcept {
	return id_;
}

void threads::TScoped::join() {
	if (id_.payload != ID::null) {
		threads::join(id_);
	}
}

void threads::init() {
	g_mainThreadID = std::this_thread::get_id();
#if defined(LEVK_OS_LINUX)
	if (XInitThreads() == 0) {
		logE("[OS] ERROR calling XInitThreads()! UB follows.");
	}
#endif
	return;
}

threads::TScoped threads::newThread(std::function<void()> task) {
	g_threads.emplace_back(++g_nextID.payload, std::thread(task));
	g_idMap[g_threads.back().second.get_id()] = g_nextID;
	return TScoped(g_nextID);
}

void threads::join(ID& id) {
	auto search = std::find_if(g_threads.begin(), g_threads.end(), [id](auto const& t) -> bool { return t.first == id.payload; });
	if (search != g_threads.end()) {
		auto& thread = search->second;
		if (thread.joinable()) {
			thread.join();
		}
		g_idMap.erase(thread.get_id());
		g_threads.erase(search);
	}
	id = {};
	return;
}

void threads::joinAll() {
	for (auto& [id, thread] : g_threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	g_idMap.clear();
	g_threads.clear();
	return;
}

threads::ID threads::thisThreadID() {
	auto search = g_idMap.find(std::this_thread::get_id());
	return search != g_idMap.end() ? search->second : ID();
}

bool threads::isMainThread() {
	return std::this_thread::get_id() == g_mainThreadID;
}

u32 threads::maxHardwareThreads() {
	return std::thread::hardware_concurrency();
}

u32 threads::runningCount() {
	return (u32)g_threads.size();
}

void threads::sleep(Time_ms duration) {
	if (duration.count() <= 0) {
		std::this_thread::yield();
	} else {
		std::this_thread::sleep_for(duration);
	}
}
} // namespace le
