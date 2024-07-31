#include <levk/core/thread_index.hpp>
#include <mutex>
#include <unordered_map>

namespace levk {
namespace {
struct {
	std::unordered_map<std::thread::id, int> map{};
	std::mutex mutex{};

	int next_id{};

	void init() {
		auto lock = std::scoped_lock{mutex};
		map.clear();
		next_id = 0;
		map.insert_or_assign(std::this_thread::get_id(), next_id++);
	}

	[[nodiscard]] auto get_or_increment_index() {
		auto lock = std::scoped_lock{mutex};
		auto it = map.find(std::this_thread::get_id());
		if (it != map.end()) { return it->second; }
		auto const ret = ++next_id;
		map.insert_or_assign(std::this_thread::get_id(), ret);
		return ret;
	}
} g_thread_id{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
} // namespace

void thread::initialize() { g_thread_id.init(); }
auto thread::get_index() -> Index { return Index{g_thread_id.get_or_increment_index()}; }
} // namespace levk
