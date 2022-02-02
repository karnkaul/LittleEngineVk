#pragma once
#include <levk/core/hash.hpp>
#include <levk/graphics/render/rgba.hpp>
#include <concepts>

namespace le {
namespace graphics {
class Texture;
} // namespace graphics
class AssetStore;

///
/// \brief Wrapper around pointer to const Texture / uri to stored Texture
///
class TextureWrap {
  public:
	using Texture = graphics::Texture;

	constexpr TextureWrap(Hash uri = {}) noexcept : m_data{{}, uri} {}
	// clang-format off
	template <typename T>
		requires std::convertible_to<T, Opt<Texture const>> 
	TextureWrap(T texture) noexcept : m_data{texture} {}
	// clang-format on

	Opt<Texture const> operator()(AssetStore const& store) const;

  private:
	struct {
		Opt<Texture const> texture{};
		Hash uri{};
	} m_data;
};

struct Material {
	///
	/// \brief Diffuse texture map
	///
	TextureWrap map_Kd{};
	///
	/// \brief Specular colour map
	///
	TextureWrap map_Ks{};
	///
	/// \brief Alpha texture map
	///
	TextureWrap map_d{};
	///
	/// \brief Normal texture map
	///
	TextureWrap map_Bump{};
	///
	/// \brief Ambient colour
	///
	graphics::RGBA Ka = colours::white;
	///
	/// \brief Diffuse colour
	///
	graphics::RGBA Kd = colours::white;
	///
	/// \brief Specular colour
	///
	graphics::RGBA Ks = colours::black;
	///
	/// \brief Transmission filter
	///
	graphics::RGBA Tf = colours::white;
	///
	/// \brief Specular coefficient
	///
	f32 Ns = 42.0f;
	///
	/// \brief Dissolve
	///
	f32 d = 1.0f;
	///
	/// \brief Illumination model
	///
	s32 illum = 2;
};
} // namespace le
