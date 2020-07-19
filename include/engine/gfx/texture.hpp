#pragma once
#include <array>
#include <string>
#include <glm/glm.hpp>
#include <core/std_types.hpp>
#include <engine/assets/asset.hpp>
#include <engine/resources/resource_types.hpp>

namespace le::gfx
{
namespace rd
{
class Set;
}

class Texture final : public Asset
{
public:
	enum class Space : s8
	{
		eSRGBNonLinear,
		eRGBLinear,
		eCOUNT_
	};
	enum class Type : s8
	{
		e2D,
		eCube,
		eCOUNT_
	};
	struct Raw final
	{
		Span<u8> bytes;
		glm::ivec2 size = {};
	};
	struct Info final
	{
		std::vector<stdfs::path> ids;
		std::vector<bytearray> bytes;
		std::vector<Raw> raws;
		stdfs::path samplerID;
		Space mode = Space::eSRGBNonLinear;
		Type type = Type::e2D;
		resources::Sampler sampler;
		class io::Reader const* pReader = nullptr;
	};

public:
	glm::ivec2 m_size = {};
	Space m_colourSpace;
	Type m_type;
	resources::Sampler m_sampler;

public:
	std::unique_ptr<struct TextureImpl> m_uImpl;

public:
	Texture(stdfs::path id, Info info);
	Texture(Texture&&);
	Texture& operator=(Texture&&);
	~Texture() override;

public:
	Status update() override;

#if defined(LEVK_ASSET_HOT_RELOAD)
protected:
	void onReload() override;
#endif
};
} // namespace le::gfx
