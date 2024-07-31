#pragma once
#include <levk/colour.hpp>
#include <levk/core/not_null.hpp>
#include <levk/core/polymorphic.hpp>
#include <levk/render_device.hpp>
#include <levk/render_target.hpp>

namespace levk {
/// \brief Execute rendering passes on given Render Targets.
class DynamicRenderer : public Polymorphic {
  public:
	enum class LoadOp { eLoad, eClear };
	enum class StoreOp { eStore, eDontCare };

	struct Transition {
		vk::ImageLayout old_layout{};
		vk::ImageLayout new_layout{};
	};

	explicit DynamicRenderer(NotNull<IRenderDevice*> device);

	/// \brief Begin rendering.
	/// \param render_target Render Target to render to.
	/// \param command_buffer Rendering command buffer.
	/// \returns true if rendering started.
	auto begin_rendering(RenderTarget const& render_target, vk::CommandBuffer command_buffer) -> bool;

	/// \brief End rendering.
	void end_rendering();

	/// \brief Incoming and outgoing image layouts
	Transition layouts{vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal};
	/// \brief Load operation for colour target when rendering begins.
	LoadOp load_op{LoadOp::eClear};
	/// \brief Store operation for depth target when rendering ends.
	StoreOp depth_store_op{StoreOp::eDontCare};
	/// \brief Clear colour value for when load_op is LoadOp::eClear.
	RgbaU8 clear_colour{colour::black_v};

  protected:
	[[nodiscard]] auto create_barrier(vk::Image image, Transition transition) const -> vk::ImageMemoryBarrier2;
	void pre_transition();
	void post_transition();

	NotNull<IRenderDevice*> m_device;

	std::vector<vk::ImageMemoryBarrier2> m_barriers{};
	RenderTarget m_render_target{};
	vk::CommandBuffer m_command_buffer{};
};
} // namespace levk
