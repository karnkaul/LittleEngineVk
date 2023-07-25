#include <spaced/engine/resources/bin_data.hpp>
#include <spaced/engine/resources/primitive_asset.hpp>

namespace spaced {
static_assert(bin::TrivialT<graphics::Vertex>);

auto PrimitiveAsset::try_load(Uri const& uri) -> bool {
	auto const bytes = read_bytes(uri);
	if (bytes.empty()) { return false; }

	auto unpack = bin::Unpack{bytes};
	auto unpacked = graphics::Geometry{};
	if (!unpack.to(unpacked.vertices)) { return false; }
	if (!unpack.bytes.empty() && !unpack.to(unpacked.indices)) { return false; }
	if (!unpack.bytes.empty() && !unpack.to(unpacked.bones)) { return false; }

	primitive.set_geometry(unpacked);
	return true;
}
} // namespace spaced
