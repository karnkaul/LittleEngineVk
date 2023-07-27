#pragma once
#include <spaced/engine/core/ptr.hpp>
#include <spaced/engine/graphics/rgba.hpp>
#include <spaced/engine/graphics/texture.hpp>
#include <spaced/engine/vfs/uri.hpp>

namespace spaced::graphics {
class Material {
  public:
	struct Shader {
		Uri vertex{};
		Uri fragment{};
	};

	Material() = default;
	Material(Material const&) = default;
	Material(Material&&) = delete;
	auto operator=(Material const&) -> Material& = default;
	auto operator=(Material&&) -> Material& = delete;

	virtual ~Material() = default;

	static auto or_default(Ptr<Material const> material) -> Material const&;

	[[nodiscard]] virtual auto get_shader() const -> Shader const& = 0;
	virtual auto bind_set(vk::CommandBuffer cmd) const -> void = 0;
};

class UnlitMaterial : public Material {
  public:
	static constexpr std::string_view material_type_v{"unlit"};

	[[nodiscard]] auto get_shader() const -> Shader const& override { return shader; }
	auto bind_set(vk::CommandBuffer cmd) const -> void override;

	Shader shader{"shaders/unlit.vert", "shaders/unlit.frag"};

	Ptr<Texture const> texture{};
};

enum class AlphaMode : std::uint32_t { eOpaque = 0, eBlend, eMask };

class LitMaterial : public Material {
  public:
	static constexpr std::string_view material_type_v{"lit"};

	struct Std140 {
		glm::vec4 albedo;
		glm::vec4 m_r_aco_am;
		glm::vec4 emissive;
	};

	[[nodiscard]] auto get_shader() const -> Shader const& override { return shader; }
	auto bind_set(vk::CommandBuffer cmd) const -> void override;

	Shader shader{"shaders/lit.vert", "shaders/lit.frag"};

	Ptr<Texture const> base_colour{};
	Ptr<Texture const> metallic_roughness{};
	Ptr<Texture const> emissive{};

	Rgba albedo{white_v};
	glm::vec3 emissive_factor{0.0f};
	// NOLINTNEXTLINE
	float metallic{0.5f};
	// NOLINTNEXTLINE
	float roughness{0.5f};
	float alpha_cutoff{};
	AlphaMode alpha_mode{AlphaMode::eOpaque};
};

class SkinnedMaterial : public LitMaterial {
  public:
	static constexpr std::string_view material_type_v{"skinned"};

	SkinnedMaterial() { shader.vertex = "shaders/skinned.vert"; }
};
} // namespace spaced::graphics
