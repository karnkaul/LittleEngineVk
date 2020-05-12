#include "core/log.hpp"
#include "engine/ecs/components.hpp"
#include "engine/ecs/registry.hpp"
#include "ecs_test.hpp"

using namespace le;

namespace test
{
s32 ecs::run()
{
	{
		Registry registry;

		// To test:
		// - flags: disabled, destroyed, debug
		//		toggle multiple objects multiple times
		// - spawn, add, get, destroy
	}
	LOG_I("[test] ECS test passed");
	return 0;
}
} // namespace test
