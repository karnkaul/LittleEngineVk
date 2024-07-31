#pragma once
#include <levk/colour.hpp>
#include <levk/material.hpp>
#include <levk/render_device.hpp>

namespace levk::material {
class Lit : public IMaterial {
  public:
	static constexpr std::string_view type_name_v{"material::Lit"};

	struct Data {
		RgbaU8 albedo{colour::white_v};
		RgbU8 emissive_factor{colour::black_v};
		float metallic{0.5f};
		float roughness{0.5f};
		float alpha_cutoff{};
		bool is_transparent{false};
	};

	explicit Lit(IRenderDevice& render_device, RenderShader const& fragment_shader);

	struct {
		std::uint32_t data{0};
		std::uint32_t base_colour{1};
		std::uint32_t metallic_roughness{2};
		std::uint32_t emissive{3};
		std::uint32_t normal{4};
	} bindings{};

	NotNull<ITexture const*> base_colour;
	NotNull<ITexture const*> metallic_roughness;
	NotNull<ITexture const*> emissive;
	Ptr<ITexture const> normal{};

	[[nodiscard]] auto get_data() const -> Data const& { return m_data; }
	void set_data(Data const& data);

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	void push_descriptors(DescriptorBuffer& descriptor_buffer) const final;

	void write_ubo() const;

	std::unique_ptr<IUniformBuffer> m_ubo{};
	Data m_data{};
};
} // namespace levk::material
