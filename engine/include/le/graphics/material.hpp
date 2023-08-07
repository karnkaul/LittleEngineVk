#pragma once
#include <le/core/ptr.hpp>
#include <le/graphics/rgba.hpp>
#include <le/graphics/shader.hpp>
#include <le/graphics/texture.hpp>

namespace le::graphics {
enum class AlphaMode : std::uint32_t {
	eOpaque = 0,
	eBlend = 1,
	eMask = 2,
};

class Material {
  public:
	Material() = default;
	Material(Material const&) = default;
	Material(Material&&) = delete;
	auto operator=(Material const&) -> Material& = default;
	auto operator=(Material&&) -> Material& = delete;

	virtual ~Material() = default;

	static auto or_default(Ptr<Material const> material) -> Material const&;

	[[nodiscard]] virtual auto get_shader() const -> Shader const& = 0;
	[[nodiscard]] virtual auto get_alpha_mode() const -> AlphaMode = 0;
	[[nodiscard]] virtual auto cast_shadow() const -> bool = 0;

	virtual auto bind_set(vk::CommandBuffer cmd) const -> void = 0;

	[[nodiscard]] auto is_transparent() const -> bool { return get_alpha_mode() == AlphaMode::eBlend; }
};

class UnlitMaterial : public Material {
  public:
	static constexpr std::string_view material_type_v{"unlit"};

	[[nodiscard]] auto get_shader() const -> Shader const& override { return shader; }
	[[nodiscard]] auto get_alpha_mode() const -> AlphaMode final { return AlphaMode::eBlend; }
	[[nodiscard]] auto cast_shadow() const -> bool final { return false; }

	auto bind_set(vk::CommandBuffer cmd) const -> void override;

	Shader shader{"shaders/unlit.vert", "shaders/unlit.frag"};

	Ptr<Texture const> texture{};
};

class LitMaterial : public Material {
  public:
	static constexpr std::string_view material_type_v{"lit"};

	struct Std140 {
		glm::vec4 albedo;
		glm::vec4 m_r_aco_am;
		glm::vec4 emissive;
	};

	[[nodiscard]] auto get_shader() const -> Shader const& override { return shader; }
	[[nodiscard]] auto get_alpha_mode() const -> AlphaMode override { return alpha_mode; }
	[[nodiscard]] auto cast_shadow() const -> bool override { return !is_transparent(); }

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

class SkyboxMaterial : public UnlitMaterial {
  public:
	static constexpr std::string_view material_type_v{"skybox"};

	SkyboxMaterial() {
		shader.vertex = "shaders/skybox.vert";
		shader.fragment = "shaders/skybox.frag";
	}
};
} // namespace le::graphics
