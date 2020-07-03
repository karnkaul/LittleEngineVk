#include <array>
#include <string>
#include <core/jobs.hpp>
#include <core/profiler.hpp>
#include <core/threads.hpp>
#include <core/maths.hpp>
#include <core/utils.hpp>
#include <engine/ecs/registry.hpp>

using namespace le;

namespace
{
struct A
{
};
struct B
{
};
struct C
{
};
struct D
{
};
struct E
{
};
struct F
{
};
} // namespace

int main()
{
	{
		jobs::Service service(4);
		Registry registry(Registry::DestroyMode::eImmediate);
		registry.m_logLevel.reset();
		constexpr s32 entityCount = 10000;
		std::array<Entity, entityCount> entities;
		s32 idx = 0;
		for (auto& entity : entities)
		{
			jobs::enqueue(
				[&entity, &registry, &idx]() {
					entity = registry.spawnEntity("e" + std::to_string(idx++));
					auto const toss = maths::randomRange(0, 1 << 7);
					if (toss & 1 << 0)
					{
						if (!registry.component<A>(entity))
						{
							registry.addComponent<A>(entity);
						}
					}
					if (toss & 1 << 1)
					{
						if (!registry.component<B>(entity))
						{
							registry.addComponent<B>(entity);
						}
					}
					if (toss & 1 << 2)
					{
						if (!registry.component<C>(entity))
						{
							registry.addComponent<C>(entity);
						}
					}
					if (toss & 1 << 3)
					{
						if (!registry.component<D>(entity))
						{
							registry.addComponent<D>(entity);
						}
					}
					if (toss & 1 << 4)
					{
						if (!registry.component<D>(entity))
						{
							registry.addComponent<D>(entity);
						}
					}
					if (toss & 1 << 5)
					{
						if (!registry.component<E>(entity))
						{
							registry.addComponent<E>(entity);
						}
					}
					if (toss & 1 << 6)
					{
						if (!registry.component<F>(entity))
						{
							registry.addComponent<F>(entity);
						}
					}
				},
				"", true);
		}
		jobs::waitIdle();
		// registry.m_logLevel = log::Level::eInfo;
		{
			for (s32 i = 0; i < entityCount / 10; ++i)
			{
				jobs::enqueue(
					[&registry, &entities]() {
						Time wait = Time(maths::randomRange(0, 3000));
						threads::sleep(wait);
						size_t const idx = (size_t)maths::randomRange(0, (s32)entities.size() - 1);
						registry.destroyEntity(entities.at(idx));
					},
					"", true);
				jobs::enqueue(
					[&registry, &entities]() {
						Time wait = Time(maths::randomRange(0, 3000));
						threads::sleep(wait);
						size_t const idx = (size_t)maths::randomRange(0, (s32)entities.size() - 1);
						registry.destroyComponent<A, B, D>(entities.at(idx));
					},
					"", true);
				jobs::enqueue(
					[&registry, &entities]() {
						Time wait = Time(maths::randomRange(0, 3000));
						threads::sleep(wait);
						size_t const idx = (size_t)maths::randomRange(0, (s32)entities.size() - 1);
						registry.setEnabled(entities.at(idx), false);
					},
					"", true);
			}
		}
		{
			constexpr s32 viewIters = 10;
			for (s32 i = 0; i < viewIters; ++i)
			{
				auto viewA = registry.view<A>();
				auto viewB = registry.view<B>();
				auto viewC = registry.view<C>();
				auto viewAB = registry.view<A, B>();
				auto viewABC = registry.view<A, B, C>();
				auto viewCEF = registry.view<C, E, F>();
			}
		}
		jobs::waitIdle();
		// To test:
		// - flags: disabled, destroyed, debug
		//		toggle multiple objects multiple times
		// - spawn, add, get, destroy
	}
	return 0;
}
