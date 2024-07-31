#pragma once
#include <levk/material.hpp>
#include <levk/render_device.hpp>

namespace levk::material {
class Unlit : public IMaterial {
  public:
	static constexpr std::string_view type_name_v{"material::Unlit"};

	explicit Unlit(IRenderDevice const& render_device, RenderShader const& fragment_shader);

	struct {
		std::uint32_t texture{0};
	} bindings{};

	NotNull<ITexture const*> texture;

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	void push_descriptors(DescriptorBuffer& descriptor_buffer) const final;
};
} // namespace levk::material
