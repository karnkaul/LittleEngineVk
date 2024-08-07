#pragma once
#include <glm/mat4x4.hpp>
#include <levk/colour.hpp>
#include <levk/core/polymorphic.hpp>
#include <levk/core/ptr.hpp>
#include <levk/core/radians.hpp>
#include <levk/lights.hpp>
#include <levk/pipeline_state.hpp>
#include <levk/render_instance.hpp>
#include <levk/render_shader.hpp>
#include <levk/render_target.hpp>
#include <levk/shader_buffer.hpp>
#include <levk/texture.hpp>

namespace levk {
struct RenderBeginInfo {
	DirectionalLight main_light{};
	glm::vec3 camera_position{};
	float camera_exposure{1.0f};
	glm::mat4 camera_view{1.0f};
	glm::mat4 camera_proj{1.0f};
	glm::mat4 shadow_view_proj{1.0f};
};

/// \brief Abstract base for RenderCameras.
/// Render Cameras generate RenderTargets - usually via owned Framebuffers.
/// They also generate projection matrices, and bind their descriptor sets for each pipeline.
class IRenderCamera : public Polymorphic {
  public:
	virtual void begin_rendering(RenderBeginInfo const& info, glm::ivec2 resolution, vk::CommandBuffer command_buffer) = 0;
	[[nodiscard]] virtual auto get_render_state() const -> RenderPassState = 0;
	[[nodiscard]] virtual auto get_view_buffer() const -> IUniformBuffer const& = 0;
	[[nodiscard]] virtual auto get_main_light_buffer() const -> IUniformBuffer const& = 0;
	[[nodiscard]] virtual auto get_render_target() const -> RenderTarget const& = 0;
	virtual auto end_rendering() -> RenderTarget = 0;

	[[nodiscard]] virtual auto get_render_texture(TextureSampler const& sampler) const -> IRenderTexture const& = 0;

	RgbaU8 clear_colour{colour::black_v};
	float render_scale{1.0f};
	vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
};
} // namespace levk
