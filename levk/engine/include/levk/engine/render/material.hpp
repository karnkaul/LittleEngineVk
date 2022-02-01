#pragma once
#include <levk/graphics/render/rgba.hpp>

namespace le {
namespace graphics {
class Texture;
} // namespace graphics

struct Material {
	///
	/// \brief Diffuse texture map
	///
	graphics::Texture const* map_Kd{};
	///
	/// \brief Specular colour map
	///
	graphics::Texture const* map_Ks{};
	///
	/// \brief Alpha texture map
	///
	graphics::Texture const* map_d{};
	///
	/// \brief Normal texture map
	///
	graphics::Texture const* map_Bump{};
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
