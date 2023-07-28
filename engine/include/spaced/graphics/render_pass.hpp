#pragma once
#include <spaced/graphics/camera.hpp>
#include <spaced/graphics/image_view.hpp>
#include <spaced/graphics/lights.hpp>
#include <spaced/graphics/pipeline_state.hpp>
#include <spaced/graphics/render_object.hpp>

namespace spaced::graphics {
struct RenderTarget {
	ImageView colour{};
	ImageView depth{};
};

struct RenderCamera {
	struct Std140View {
		glm::mat4 mat_vp;
		glm::vec4 vpos_exposure;
	};

	struct Std430DirLight {
		glm::vec4 direction;
		glm::vec4 diffuse;
		glm::vec4 ambient;
	};

	auto bind_set(glm::vec2 projection, vk::CommandBuffer cmd) const -> void;

	NotNull<Camera const*> camera;
	NotNull<Lights const*> lights;
};

class RenderPass {
  public:
	RenderPass() = default;
	RenderPass(RenderPass const&) = default;
	RenderPass(RenderPass&&) = default;
	auto operator=(RenderPass const&) -> RenderPass& = default;
	auto operator=(RenderPass&&) -> RenderPass& = default;

	virtual ~RenderPass() = default;

	[[nodiscard]] auto swapchain_image() const -> ImageView const& { return m_data.swapchain_image; }
	[[nodiscard]] auto framebuffer_extent() const -> glm::uvec2 { return {m_data.swapchain_image.extent.width, m_data.swapchain_image.extent.height}; }

	vk::ClearColorValue clear_colour{};

  protected:
	using Object = RenderObject;
	using Instance = RenderInstance;

	virtual auto render(vk::CommandBuffer cmd) -> void = 0;

	virtual auto setup_framebuffer() -> void {}

	auto set_colour(ImageView const& colour) -> void { m_data.render_target.colour = colour; }
	auto set_depth(ImageView const& depth) -> void { m_data.render_target.depth = depth; }

	[[nodiscard]] auto get_projection() const -> glm::vec2 { return m_data.projection; }
	auto set_projection(glm::vec2 extent) -> void { m_data.projection = extent; }

	auto render_objects(RenderCamera const& camera, std::span<RenderObject const> objects, vk::CommandBuffer cmd) -> void;

	vk::AttachmentLoadOp m_colour_load_op{vk::AttachmentLoadOp::eClear};

  private:
	struct Data {
		RenderTarget render_target{};
		ImageView swapchain_image{};
		glm::vec2 projection{};
		Ptr<Material const> last_bound{};
	};

	struct Std430Instance {
		glm::mat4 transform;
		glm::vec4 tint;
	};

	auto do_setup(RenderTarget const& swapchain) -> void;
	auto do_render(vk::CommandBuffer cmd) -> void;

	Data m_data{};
	std::vector<Std430Instance> m_instances{};

	friend class Renderer;
};
} // namespace spaced::graphics
