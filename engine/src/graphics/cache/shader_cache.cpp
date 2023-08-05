#include <le/core/logger.hpp>
#include <le/graphics/cache/shader_cache.hpp>
#include <le/graphics/device.hpp>
#include <le/vfs/vfs.hpp>

namespace le::graphics {
namespace {
auto to_spir_v(Uri const& uri) -> Uri {
	if (uri.extension() == ".spv") { return uri; }
	return std::string{uri} + ".spv";
}

auto const g_log{logger::Logger{"Cache"}};
} // namespace

auto ShaderCache::load(Uri const& uri) -> vk::ShaderModule {
	if (auto it = m_modules.find(uri); it != m_modules.end()) { return *it->second; }

	auto const bytes = vfs::read_bytes(to_spir_v(uri));
	if (bytes.empty()) { return {}; }
	auto const smci = vk::ShaderModuleCreateInfo{
		{},
		bytes.size(),
		// NOLINTNEXTLINE
		reinterpret_cast<std::uint32_t const*>(bytes.data()),
	};
	auto shader_module = Device::self().get_device().createShaderModuleUnique(smci);
	if (!shader_module) { return {}; }

	auto [it, _] = m_modules.insert_or_assign(uri, std::move(shader_module));
	g_log.debug("new Vulkan Shader Module created [{}] (total: {})", uri.value(), m_modules.size());

	return *it->second;
}

auto ShaderCache::clear_shaders() -> void {
	g_log.debug("[{}] Vulkan Shader Modules destroyed", m_modules.size());
	m_modules.clear();
}
} // namespace le::graphics
