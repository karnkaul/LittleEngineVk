#pragma once
#include <spaced/core/inclusive_range.hpp>
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
		glm::mat4 view;
		glm::mat4 projection;
		glm::vec4 vpos_exposure;
		glm::vec4 vdir_ortho;
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

class Subpass {
  public:
	struct Load {
		vk::ClearColorValue clear_colour{std::array{0.0f, 0.0f, 0.0f, 1.0f}};
		vk::AttachmentLoadOp load_op{vk::AttachmentLoadOp::eClear};
	};

	Subpass() = default;
	Subpass(Subpass const&) = default;
	Subpass(Subpass&&) = default;
	auto operator=(Subpass const&) -> Subpass& = default;
	auto operator=(Subpass&&) -> Subpass& = default;

	virtual ~Subpass() = default;

	[[nodiscard]] auto swapchain_image() const -> ImageView const& { return m_data.swapchain_image; }
	[[nodiscard]] auto framebuffer_extent() const -> glm::uvec2 { return {m_data.swapchain_image.extent.width, m_data.swapchain_image.extent.height}; }

  protected:
	using Object = RenderObject;
	using Instance = RenderInstance;

	[[nodiscard]] virtual auto get_load() const -> Load { return {}; }
	virtual auto render(vk::CommandBuffer cmd) -> void = 0;

	[[nodiscard]] auto get_projection() const -> glm::vec2 { return m_data.projection; }
	auto set_projection(glm::vec2 extent) -> void { m_data.projection = extent; }

	auto render_objects(RenderCamera const& camera, std::span<RenderObject const> objects, vk::CommandBuffer cmd) -> void;

  private:
	struct Data {
		RenderTarget render_target{};
		ImageView swapchain_image{};
		glm::vec2 projection{};
		Ptr<Material const> last_bound{};
		InclusiveRange<float> line_width_limit{};
	};

	struct Std430Instance {
		glm::mat4 transform;
		glm::vec4 tint;
	};

	auto do_setup(RenderTarget const& swapchain, InclusiveRange<float> line_width_limit) -> void;
	auto do_render(vk::CommandBuffer cmd) -> void;

	Data m_data{};
	std::vector<Std430Instance> m_instances{};

	friend class Renderer;
};
} // namespace spaced::graphics
