#pragma once
#include <variant>
#include <glm/vec2.hpp>
#include <graphics/context/vram.hpp>

namespace le::graphics {
class Texture {
  public:
	enum class Type { e2D, e3D };
	struct Data {
		View<Image> image;
		vk::ImageView imageView;
		vk::Sampler sampler;
		vk::Format format;
		glm::ivec2 size = {};
		Type type;
	};
	struct Compressed;
	struct Raw;
	struct CreateInfo;

	Texture(std::string name, VRAM& vram);
	Texture(Texture&&);
	Texture& operator=(Texture&&);
	virtual ~Texture();

	bool construct(CreateInfo const& info);
	void destroy();
	bool valid() const;
	bool busy() const;
	bool ready() const;
	void wait();

	Data const& data() const noexcept;

	std::string m_name;

  protected:
	struct Storage {
		Data data;
		struct {
			std::vector<Span<u8>> bytes;
			std::vector<RawImage> imgs;
		} raw;
		VRAM::Future transfer;
	};
	Storage m_storage;

  private:
	Ref<VRAM> m_vram;
};
struct Texture::Compressed {
	std::vector<bytearray> bytes;
};
struct Texture::Raw {
	std::vector<u8> bytes;
	glm::ivec2 size = {};
};
struct Texture::CreateInfo {
	std::variant<Compressed, Raw> data;
	vk::Sampler sampler;
	vk::Format format = vk::Format::eR8G8B8A8Srgb;
	Type type = Type::e2D;
};
} // namespace le::graphics
