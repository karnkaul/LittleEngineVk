#include <levk/asset_store.hpp>
#include <levk/assets/shader_asset.hpp>
#include <filesystem>

namespace levk {
namespace {
namespace fs = std::filesystem;

[[nodiscard]] auto get_spirv_uri(fs::path uri) {
	if (uri.extension().string() != ".spv") { uri += ".spv"; }
	return uri.generic_string();
}
} // namespace

auto ShaderAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	m_spirv = store.get_vfs().load_bytes(get_spirv_uri(info.uri));
	return !m_spirv.empty() && m_spirv.size() % sizeof(std::uint32_t) == 0;
}
} // namespace levk
