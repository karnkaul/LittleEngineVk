#pragma once
#include "engine/vuk/instance/instance.hpp"
#include "engine/vuk/instance/device.hpp"

namespace le::vuk
{
inline Instance::Data g_instanceData;
inline std::unique_ptr<Instance> g_uInstance;
inline Device* g_pDevice = nullptr;
} // namespace le::vuk
