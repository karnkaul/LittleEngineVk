#include <spaced/graphics/device.hpp>
#include <spaced/resources/shader_asset.hpp>

namespace spaced {
namespace {
auto to_spir_v(Uri const& uri) -> Uri {
	if (uri.extension() == ".spv") { return uri; }
	return std::string{uri} + ".spv";
}
} // namespace

auto ShaderAsset::try_load(Uri const& uri) -> bool {
	auto const bytes = read_bytes(to_spir_v(uri));
	if (bytes.empty()) { return false; }
	auto const smci = vk::ShaderModuleCreateInfo{
		{},
		bytes.size(),
		// NOLINTNEXTLINE
		reinterpret_cast<std::uint32_t const*>(bytes.data()),
	};
	module = graphics::Device::self().device().createShaderModuleUnique(smci);
	return static_cast<bool>(module);
}
} // namespace spaced
