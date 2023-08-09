#include <le/resources/pcm_asset.hpp>

namespace le {
auto PcmAsset::try_load(Uri const& uri) -> bool {
	auto bytes = read_bytes(uri);
	if (bytes.empty()) { return false; }

	auto const guessed_compression = capo::to_compression(uri.extension());
	auto result = capo::Pcm::make(bytes, guessed_compression);
	if (!result) { return false; }

	pcm = std::move(result.pcm);
	compression = result.compression;

	return true;
}
} // namespace le
