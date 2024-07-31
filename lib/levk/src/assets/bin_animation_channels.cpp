#include <levk/assets/bin_animation_channels.hpp>
#include <levk/io/binary_io.hpp>

namespace {
enum class Type : std::int8_t { ePosition, eOrientation, eScale };

template <typename SamplerT>
constexpr auto get_type(SamplerT const& /*t*/) {
	if constexpr (std::same_as<SamplerT, levk::TransformSampler::Translator>) {
		return Type::ePosition;
	} else if constexpr (std::same_as<SamplerT, levk::TransformSampler::Rotator>) {
		return Type::eOrientation;
	} else if constexpr (std::same_as<SamplerT, levk::TransformSampler::Scaler>) {
		return Type::eScale;
	}

	std::unreachable();
}

struct Metadata {
	Type type{};
	levk::LerpType lerp_type{};
	levk::ImportIndex import_index{};
	std::int32_t count{};

	[[nodiscard]] static auto from(levk::binary::Payload const& payload) {
		auto ret = Metadata{};
		std::memcpy(&ret, payload.data(), sizeof(Metadata));
		return ret;
	}

	[[nodiscard]] auto to_payload() const -> levk::binary::Payload {
		auto ret = levk::binary::Payload{};
		std::memcpy(ret.data(), this, sizeof(Metadata));
		return ret;
	}
};

static_assert(sizeof(Metadata) <= sizeof(levk::binary::Payload));

[[nodiscard]] auto get_size_bytes(levk::TransformSampler const& sampler) {
	auto const visitor = [](auto const& sampler) { return sampler.get_keyframes().size_bytes(); };
	return std::visit(visitor, sampler.sampler);
}

template <levk::binary::BinaryDataT Type>
struct Keyframe {
	Type output;
	float timestamp;
};

struct WriteBytes {
	levk::binary::Writer& writer; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
	levk::ImportIndex target{};

	template <typename Type>
	void operator()(Type const& sampler) const {
		using kf_t = Keyframe<typename Type::value_type>;
		auto const in_keyframes = sampler.get_keyframes();
		auto out_keyframes = std::vector<kf_t>{};
		out_keyframes.reserve(in_keyframes.size());
		for (auto const& keyframe : in_keyframes) { out_keyframes.push_back({.output = keyframe.output, .timestamp = keyframe.timestamp.count()}); }

		auto const metadata = Metadata{
			.type = get_type(sampler),
			.lerp_type = sampler.lerp_type,
			.import_index = target,
			.count = static_cast<std::int32_t>(sampler.get_keyframes().size()),
		};
		auto const payload = metadata.to_payload();
		writer.write_next(std::span<kf_t const>{out_keyframes}, payload);
	}
};

struct ReadBytes {
	levk::TransformSampler& out; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

	void operator()(levk::binary::Reader& reader) const {
		auto const* header = reader.get_current_header();
		if (header == nullptr) { return; }
		auto const metadata = Metadata::from(header->payload);
		switch (metadata.type) {
		case Type::ePosition: read_vec3<levk::TransformSampler::Translator>(reader, metadata); break;
		case Type::eOrientation: read_quat(reader, metadata); break;
		case Type::eScale: read_vec3<levk::TransformSampler::Scaler>(reader, metadata); break;
		default: break;
		}
	}

	template <typename Type>
	void read_vec3(levk::binary::Reader& reader, Metadata const& metadata) const {
		auto in_keyframes = std::vector<Keyframe<glm::vec3>>{};
		if (!reader.read_next(in_keyframes)) { return; }
		if (metadata.count != static_cast<std::int32_t>(in_keyframes.size())) { return; }
		auto sampler = Type{};
		sampler.reserve(in_keyframes.size());
		sampler.lerp_type = metadata.lerp_type;
		for (auto const& in_keyframe : in_keyframes) { sampler.add_keyframe(levk::Seconds{in_keyframe.timestamp}, in_keyframe.output); }
		out.sampler = std::move(sampler);
	}

	void read_quat(levk::binary::Reader& reader, Metadata const& metadata) const {
		auto in_keyframes = std::vector<Keyframe<glm::quat>>{};
		if (!reader.read_next(in_keyframes)) { return; }
		if (metadata.count != static_cast<std::int32_t>(in_keyframes.size())) { return; }
		auto sampler = levk::TransformSampler::Rotator{};
		sampler.lerp_type = metadata.lerp_type;
		sampler.reserve(in_keyframes.size());
		for (auto const& in_keyframe : in_keyframes) { sampler.add_keyframe(levk::Seconds{in_keyframe.timestamp}, in_keyframe.output); }
		out.sampler = std::move(sampler);
	}
};

struct ReadBytes2 {
	std::vector<levk::TransformSampler>& out; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

	void operator()(levk::binary::Reader& reader) const {
		auto const* header = reader.get_current_header();
		while (header != nullptr) {
			auto const metadata = Metadata::from(header->payload);
			switch (metadata.type) {
			case Type::ePosition: read_vec3<levk::TransformSampler::Translator>(reader, metadata); break;
			case Type::eOrientation: read_quat(reader, metadata); break;
			case Type::eScale: read_vec3<levk::TransformSampler::Scaler>(reader, metadata); break;
			default: return;
			}
			header = reader.get_current_header();
		}
	}

	template <typename Type>
	void read_vec3(levk::binary::Reader& reader, Metadata const& metadata) const {
		auto in_keyframes = std::vector<Keyframe<glm::vec3>>{};
		if (!reader.read_next(in_keyframes)) { return; }
		if (metadata.count != static_cast<std::int32_t>(in_keyframes.size())) { return; }
		auto sampler = Type{};
		sampler.reserve(in_keyframes.size());
		sampler.lerp_type = metadata.lerp_type;
		for (auto const& in_keyframe : in_keyframes) { sampler.add_keyframe(levk::Seconds{in_keyframe.timestamp}, in_keyframe.output); }
		out.push_back(levk::TransformSampler{.sampler = std::move(sampler)});
	}

	void read_quat(levk::binary::Reader& reader, Metadata const& metadata) const {
		auto in_keyframes = std::vector<Keyframe<glm::quat>>{};
		if (!reader.read_next(in_keyframes)) { return; }
		if (metadata.count != static_cast<std::int32_t>(in_keyframes.size())) { return; }
		auto sampler = levk::TransformSampler::Rotator{};
		sampler.lerp_type = metadata.lerp_type;
		sampler.reserve(in_keyframes.size());
		for (auto const& in_keyframe : in_keyframes) { sampler.add_keyframe(levk::Seconds{in_keyframe.timestamp}, in_keyframe.output); }
		out.push_back(levk::TransformSampler{.sampler = std::move(sampler)});
	}
};

struct ReadBytes3 {
	std::vector<levk::AnimationChannel>& out; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
	levk::binary::Reader& reader;			  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

	void operator()() const {
		auto const* header = reader.get_current_header();
		while (header != nullptr) {
			auto const metadata = Metadata::from(header->payload);
			switch (metadata.type) {
			case Type::ePosition: push_translator(metadata); break;
			case Type::eOrientation: push_rotator(metadata); break;
			case Type::eScale: push_scaler(metadata); break;
			default: return;
			}
			header = reader.get_current_header();
		}
	}

	template <typename Type>
	void push_vec3(Metadata const& metadata) const {
		auto in_keyframes = std::vector<Keyframe<glm::vec3>>{};
		if (!reader.read_next(in_keyframes)) { return; }
		if (metadata.count != static_cast<std::int32_t>(in_keyframes.size())) { return; }
		auto sampler = Type{};
		sampler.reserve(in_keyframes.size());
		sampler.lerp_type = metadata.lerp_type;
		for (auto const& in_keyframe : in_keyframes) { sampler.add_keyframe(levk::Seconds{in_keyframe.timestamp}, in_keyframe.output); }
		auto& channel = out.emplace_back();
		channel.sampler = levk::TransformSampler{.sampler = std::move(sampler)};
		channel.target = metadata.import_index;
	}

	void push_translator(Metadata const& metadata) const { push_vec3<levk::TransformSampler::Translator>(metadata); }

	void push_scaler(Metadata const& metadata) const { push_vec3<levk::TransformSampler::Scaler>(metadata); }

	void push_rotator(Metadata const& metadata) const {
		auto in_keyframes = std::vector<Keyframe<glm::quat>>{};
		if (!reader.read_next(in_keyframes)) { return; }
		if (metadata.count != static_cast<std::int32_t>(in_keyframes.size())) { return; }
		auto sampler = levk::TransformSampler::Rotator{};
		sampler.lerp_type = metadata.lerp_type;
		sampler.reserve(in_keyframes.size());
		for (auto const& in_keyframe : in_keyframes) { sampler.add_keyframe(levk::Seconds{in_keyframe.timestamp}, in_keyframe.output); }
		auto& channel = out.emplace_back();
		channel.sampler = levk::TransformSampler{.sampler = std::move(sampler)};
		channel.target = metadata.import_index;
	}
};
} // namespace

void levk::to_bytes(std::vector<std::byte>& out, std::span<AnimationChannel const> animation_channels) {
	auto writer = levk::binary::Writer{out};
	for (auto const& channel : animation_channels) {
		out.reserve(out.size() + sizeof(levk::binary::Header) + sizeof(channel.target) + get_size_bytes(channel.sampler));
		std::visit(WriteBytes{writer, channel.target}, channel.sampler.sampler);
	}
}

void levk::from_bytes(std::span<std::byte const> bytes, std::vector<AnimationChannel>& out) {
	auto reader = levk::binary::Reader{bytes};
	ReadBytes3{out, reader}();
}
