#pragma once
#include <spaced/engine/core/mono_instance.hpp>
#include <spaced/engine/core/ptr.hpp>
#include <spaced/engine/graphics/texture.hpp>

namespace spaced::graphics {
class Fallback : public MonoInstance<Fallback> {
  public:
	Fallback();

	[[nodiscard]] auto white_texture() const -> Texture const& { return m_textures.white; }
	[[nodiscard]] auto black_texture() const -> Texture const& { return m_textures.black; }
	[[nodiscard]] auto white_cubemap() const -> Cubemap const& { return m_cubemaps.white; }
	[[nodiscard]] auto black_cubemap() const -> Cubemap const& { return m_cubemaps.black; }

	[[nodiscard]] auto or_white(Ptr<Texture const> in) const -> Texture const& { return in != nullptr ? *in : white_texture(); }
	[[nodiscard]] auto or_black(Ptr<Texture const> in) const -> Texture const& { return in != nullptr ? *in : black_texture(); }
	[[nodiscard]] auto or_white(Ptr<Cubemap const> in) const -> Cubemap const& { return in != nullptr ? *in : white_cubemap(); }
	[[nodiscard]] auto or_black(Ptr<Cubemap const> in) const -> Cubemap const& { return in != nullptr ? *in : black_cubemap(); }

  private:
	struct {
		Texture white{};
		Texture black{};
	} m_textures{};

	struct {
		Cubemap white{};
		Cubemap black{};
	} m_cubemaps{};
};
} // namespace spaced::graphics
