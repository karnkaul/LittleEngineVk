#include <array>
#include <string>
#include <unordered_set>
#include <core/ensure.hpp>
#include <core/maths.hpp>
#include <core/threads.hpp>
#include <core/utils/algo.hpp>
#include <dtasks/task_scheduler.hpp>
#include <dumb_ecf/registry.hpp>
#include <kt/async_queue/lockable.hpp>

using namespace le;
using namespace decf;

namespace {
struct A {};
struct B {};
struct C {};
struct D {};
struct E {};
struct F {};

std::unordered_set<entity_t> g_spawned;
kt::lockable_t<> g_mutex;

bool verify(entity_t entity) {
	auto lock = g_mutex.lock();
	bool const bRet = !utils::contains(g_spawned, entity);
	ENSURE(bRet, "DUPLICATE");
	g_spawned.insert(entity);
	return bRet;
}
} // namespace

int main() {
	{
		dts::task_queue tq;
		registry_t registry;
		constexpr s32 entityCount = 10000;
		std::array<entity_t, entityCount> entities;
		s32 idx = 0;
		bool bPass = true;
		for (auto& entity : entities) {
			tq.enqueue([&entity, &registry, &idx, &bPass]() {
				entity = registry.spawn("e" + std::to_string(idx++));
				bPass &= verify(entity);
				auto const toss = maths::randomRange(0, 1 << 7);
				if (toss & 1 << 0) {
					if (!registry.find<A>(entity)) {
						registry.attach<A>(entity);
					}
				}
				if (toss & 1 << 1) {
					if (!registry.find<B>(entity)) {
						registry.attach<B>(entity);
					}
				}
				if (toss & 1 << 2) {
					if (!registry.find<C>(entity)) {
						registry.attach<C>(entity);
					}
				}
				if (toss & 1 << 3) {
					if (!registry.find<D>(entity)) {
						registry.attach<D>(entity);
					}
				}
				if (toss & 1 << 4) {
					if (!registry.find<D>(entity) && !registry.find<C>(entity)) {
						registry.attach<D>(entity);
						registry.attach<D>(entity);
					}
				}
				if (toss & 1 << 5) {
					if (!registry.find<E>(entity)) {
						registry.attach<E>(entity);
					}
				}
				if (toss & 1 << 6) {
					if (!registry.find<F>(entity)) {
						registry.attach<F>(entity);
					}
				}
			});
		}
		tq.wait_idle();
		if (!bPass) {
			return 1;
		}
		std::vector<dts::task_id> handles;
		{
			auto wait = []() -> Time_ms { return time::cast<Time_ms>(Time_us(maths::randomRange(0, 3000))); };
			for (s32 i = 0; i < entityCount / 10; ++i) {
				handles.push_back(tq.enqueue([&registry, &entities, wait]() {
					threads::sleep(wait());
					std::size_t const idx = (std::size_t)maths::randomRange(0, (s32)entities.size() - 1);
					registry.destroy(entities[idx]);
				}));
				handles.push_back(tq.enqueue([&registry, &entities, wait]() {
					threads::sleep(wait());
					std::size_t const idx = (std::size_t)maths::randomRange(0, (s32)entities.size() - 1);
					registry.detach<A>(entities[idx]);
					registry.detach<B>(entities[idx]);
					registry.detach<D>(entities[idx]);
				}));
				handles.push_back(tq.enqueue([&registry, &entities, wait]() {
					threads::sleep(wait());
					std::size_t const idx = (std::size_t)maths::randomRange(0, (s32)entities.size() - 1);
					registry.enable(entities[idx], false);
				}));
			}
		}
		{
			constexpr s32 viewIters = 10;
			for (s32 i = 0; i < viewIters; ++i) {
				[[maybe_unused]] auto viewA = registry.view<A>();
				[[maybe_unused]] auto viewB = registry.view<B>();
				[[maybe_unused]] auto viewC = registry.view<C>();
				[[maybe_unused]] auto viewAB = registry.view<A, B>();
				[[maybe_unused]] auto viewABC = registry.view<A, B, C>();
				[[maybe_unused]] auto viewCEF = registry.view<C, E, F>();
			}
		}
		tq.wait_tasks(handles);
		// To test:
		// - flags: disabled, destroyed, debug
		//		toggle multiple objects multiple times
		// - spawn, add, get, destroy
	}
	return 0;
}
