#include <levk/asset_store.hpp>
#include <levk/assets/image_asset.hpp>
#include <levk/image_file.hpp>

namespace levk {
auto ImageAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const bytes = store.get_vfs().load_bytes(info.uri);
	if (bytes.empty()) { return false; }

	auto image_file = ImageFile{};
	if (!image_file.load_from_bytes(bytes)) { return false; }

	image = std::move(image_file);
	return true;
}
} // namespace levk
