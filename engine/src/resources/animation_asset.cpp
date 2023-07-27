#include <spaced/engine/core/visitor.hpp>
#include <spaced/engine/resources/animation_asset.hpp>
#include <spaced/engine/resources/bin_data.hpp>

namespace spaced {
namespace {
constexpr auto bin_sign_v{BinSign{0xffff0001}};

enum class SamplerType : std::uint8_t { eTranslate, eRotate, eScale };

struct SamplerHeader {
	SamplerType type;
	graphics::Interpolation interpolation;
	std::uint64_t joint_id;
	std::uint64_t count;
};

struct BinChannel {
	Id<Node>::id_type joint_id{};
	std::variant<graphics::Animation::Translate, graphics::Animation::Rotate, graphics::Animation::Scale> storage{};
};

template <typename T>
auto pack_channel(BinWriter& out, SamplerType type, Id<Node> const joint_id, T const& in) -> void {
	auto const header = SamplerHeader{
		.type = type,
		.interpolation = in.interpolation,
		.joint_id = joint_id,
		.count = in.keyframes.size(),
	};
	out.write(std::span{&header, 1}).write(std::span{in.keyframes});
}

template <typename T>
auto channel_storage_from(BinReader& out, SamplerHeader const& header, BinChannel& out_channel) -> bool {
	auto ret = T{};
	ret.interpolation = header.interpolation;
	ret.keyframes.resize(header.count);
	if (!out.read(std::span{ret.keyframes})) { return false; }
	out_channel.storage = std::move(ret);
	return true;
}

auto unpack_channel(BinReader& out, BinChannel& out_channel) -> bool {
	auto header = SamplerHeader{};
	if (!out.read(std::span{&header, 1})) { return false; }
	out_channel.joint_id = header.joint_id;
	switch (header.type) {
	case SamplerType::eTranslate: return channel_storage_from<graphics::Animation::Translate>(out, header, out_channel);
	case SamplerType::eRotate: return channel_storage_from<graphics::Animation::Rotate>(out, header, out_channel);
	case SamplerType::eScale: return channel_storage_from<graphics::Animation::Scale>(out, header, out_channel);
	default: return false;
	}
}

auto pack_channel(BinWriter& out, graphics::Animation::Channel const& channel) -> void {
	auto const visitor = Visitor{
		[&](graphics::Animation::Translate const& translate) { pack_channel(out, SamplerType::eTranslate, channel.joint_id, translate); },
		[&](graphics::Animation::Rotate const& rotate) { pack_channel(out, SamplerType::eRotate, channel.joint_id, rotate); },
		[&](graphics::Animation::Scale const& scale) { pack_channel(out, SamplerType::eScale, channel.joint_id, scale); },
	};
	std::visit(visitor, channel.storage);
}
} // namespace

auto AnimationAsset::try_load(Uri const& uri) -> bool {
	auto bytes = read_bytes(uri);
	if (bytes.empty()) { return false; }

	return bin_unpack_from(bytes, animation);
}

auto AnimationAsset::bin_pack_to(std::vector<std::uint8_t>& out, graphics::Animation::Channel const& channel) -> void {
	auto writer = BinWriter{out};
	pack_channel(writer, channel);
}

auto AnimationAsset::bin_unpack_from(std::span<std::uint8_t const> bytes, graphics::Animation::Channel& out) -> bool {
	auto reader = BinReader{bytes};
	auto bin_channel = BinChannel{};
	if (!unpack_channel(reader, bin_channel)) { return false; }
	out.joint_id = bin_channel.joint_id;
	out.storage = std::move(bin_channel.storage);
	return true;
}

auto AnimationAsset::bin_pack_to(std::vector<std::uint8_t>& out, graphics::Animation const& animation) -> void {
	auto writer = BinWriter{out};
	writer.write(std::span{&bin_sign_v, 1});
	auto count = static_cast<std::uint64_t>(animation.channels.size());
	writer.write(std::span{&count, 1});
	for (auto const& channel : animation.channels) { pack_channel(writer, channel); }
}

auto AnimationAsset::bin_unpack_from(std::span<std::uint8_t const> bytes, graphics::Animation& out) -> bool {
	auto reader = BinReader{bytes};
	auto sign = BinSign{};
	if (!reader.read(std::span{&sign, 1}) || sign != bin_sign_v) { return false; }
	auto count = std::uint64_t{};
	if (!reader.read(std::span{&count, 1})) { return false; }
	if (count == 0) { return true; }
	auto start = out.channels.size();
	auto bin_channels = std::vector<BinChannel>(count);
	auto const span = std::span{bin_channels}.subspan(start);
	out.channels.reserve(out.channels.size() + count);
	for (auto& channel : span) {
		if (!unpack_channel(reader, channel)) { return false; }
		out.channels.push_back({channel.joint_id, std::move(channel.storage)});
	}
	return true;
}
} // namespace spaced
