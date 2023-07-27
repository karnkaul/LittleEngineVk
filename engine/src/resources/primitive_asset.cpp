#include <spaced/engine/resources/bin_data.hpp>
#include <spaced/engine/resources/primitive_asset.hpp>
#include <spaced/engine/vfs/file_reader.hpp>

namespace spaced {
static_assert(BinaryT<graphics::Vertex>);

namespace {
constexpr auto bin_sign_v{BinSign{0xffff0001}};
} // namespace

auto PrimitiveAsset::try_load(Uri const& uri) -> bool {
	auto const bytes = read_bytes(uri);
	if (bytes.empty()) { return false; }

	auto reader = BinReader{bytes};
	auto unpacked = graphics::Geometry{};

	auto sign = BinSign{};
	if (!reader.read<BinSign>({&sign, 1}) || sign != bin_sign_v) { return false; }

	auto count = std::uint64_t{};
	if (!reader.read(std::span{&count, 1})) { return false; }
	unpacked.vertices.resize(count);
	if (!reader.read(std::span{unpacked.vertices})) { return false; }
	if (!reader.read(std::span{&count, 1})) { return false; }
	unpacked.indices.resize(count);
	if (!reader.read(std::span{unpacked.indices})) { return false; }
	if (!reader.read(std::span{&count, 1})) { return false; }
	unpacked.bones.resize(count);
	if (!reader.read(std::span{unpacked.bones})) { return false; }

	primitive.set_geometry(unpacked);
	return true;
}

auto PrimitiveAsset::bin_pack_to(std::vector<std::uint8_t>& out, graphics::Geometry const& geometry) -> void {
	auto writer = BinWriter{out};
	writer.write(std::span{&bin_sign_v, 1});

	auto count = geometry.vertices.size();
	writer.write(std::span{&count, 1}).write(std::span{geometry.vertices});
	count = geometry.indices.size();
	writer.write(std::span{&count, 1}).write(std::span{geometry.indices});
	count = geometry.bones.size();
	writer.write(std::span{&count, 1}).write(std::span{geometry.bones});
}
} // namespace spaced
