#pragma once
#include <functional>
#include "core/std_types.hpp"
#include "core/zero.hpp"

namespace le
{
using HThread = TZero<s32>;

namespace threads
{
HThread newThread(std::function<void()> task);
void join(HThread& id);
void joinAll();

u32 maxHardwareThreads();
u32 running();
} // namespace threads
} // namespace le
