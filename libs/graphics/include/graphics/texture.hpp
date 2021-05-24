#pragma once
#include <variant>
#include <glm/vec2.hpp>
#include <graphics/bitmap.hpp>
#include <graphics/context/vram.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>

namespace le::graphics {
class Sampler {
  public:
	using MinMag = std::pair<vk::Filter, vk::Filter>;
	static vk::SamplerCreateInfo info(MinMag minMag, vk::SamplerMipmapMode mip = vk::SamplerMipmapMode::eLinear);

	Sampler(not_null<Device*> device, vk::SamplerCreateInfo const& info);
	Sampler(not_null<Device*> device, MinMag minMag, vk::SamplerMipmapMode mip = vk::SamplerMipmapMode::eLinear);
	Sampler(Sampler&&);
	Sampler& operator=(Sampler&&);
	virtual ~Sampler();

	vk::Sampler sampler() const noexcept { return m_sampler; }

  private:
	void destroy();

	vk::Sampler m_sampler;
	not_null<Device*> m_device;
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

	using Img = Bitmap::type;
	using Cubemap = std::array<Bitmap::type, 6>;
	struct CreateInfo;

	inline static constexpr auto srgb = vk::Format::eR8G8B8A8Srgb;
	inline static constexpr auto linear = vk::Format::eR8G8B8A8Unorm;

	Texture(not_null<VRAM*> vram);
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

	not_null<VRAM*> m_vram;

  private:
	struct Storage {
		std::optional<Image> image;
		Data data;
		VRAM::Future transfer;
	};

	bool construct(CreateInfo const& info, Storage& out_storage);
	void destroy();

	Storage m_storage;
};

struct Texture::CreateInfo {
	using Data = std::variant<Img, Cubemap, Bitmap>;

	Data data;
	vk::Sampler sampler;
	std::optional<vk::Format> forceFormat;
};
} // namespace le::graphics
