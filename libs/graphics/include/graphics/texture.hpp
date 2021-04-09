#pragma once
#include <variant>
#include <glm/vec2.hpp>
#include <graphics/context/vram.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>

namespace le::graphics {
class Sampler {
  public:
	using MinMag = std::pair<vk::Filter, vk::Filter>;
	static vk::SamplerCreateInfo info(MinMag minMag, vk::SamplerMipmapMode mip = vk::SamplerMipmapMode::eLinear);

	Sampler(Device& device, vk::SamplerCreateInfo const& info);
	Sampler(Device& device, MinMag minMag, vk::SamplerMipmapMode mip = vk::SamplerMipmapMode::eLinear);
	Sampler(Sampler&&);
	Sampler& operator=(Sampler&&);
	virtual ~Sampler();

	vk::Sampler sampler() const noexcept {
		return m_sampler;
	}

  private:
	void destroy();

	vk::Sampler m_sampler;
	Ref<Device> m_device;
};

class Texture {
  public:
	enum class Type { e2D, eCube };
	struct Data {
		vk::ImageView imageView;
		vk::Sampler sampler;
		vk::Format format;
		glm::ivec2 size = {};
		Type type;
	};
	struct RawImage {
		View<std::byte> bytes;
		int width = 0;
		int height = 0;
	};

	struct Img;
	struct Cubemap;
	struct Raw;
	struct CreateInfo;

	inline static constexpr auto srgbFormat = vk::Format::eR8G8B8A8Srgb;

	Texture(VRAM& vram);
	Texture(Texture&&);
	Texture& operator=(Texture&&);
	virtual ~Texture();

	bool construct(CreateInfo const& info);

	bool valid() const noexcept;
	bool busy() const;
	bool ready() const;
	void wait() const;

	Data const& data() const noexcept;
	Image const& image() const;

	Ref<VRAM> m_vram;

  private:
	struct Storage {
		Data data;
		std::optional<Image> image;
		struct {
			kt::fixed_vector<View<std::byte>, 6> bytes;
			kt::fixed_vector<RawImage, 6> imgs;
		} raw;
		VRAM::Future transfer;
	};

	bool construct(CreateInfo const& info, Storage& out_storage);
	void destroy();

	Storage m_storage;
};

struct Texture::Img {
	bytearray bytes;
};
struct Texture::Cubemap {
	std::array<bytearray, 6> bytes;
};
struct Texture::Raw {
	bytearray bytes;
	glm::ivec2 size = {};
};
struct Texture::CreateInfo {
	using Data = std::variant<Img, Cubemap, Raw>;

	Data data;
	vk::Sampler sampler;
	vk::Format format = Texture::srgbFormat;
};
} // namespace le::graphics
