#pragma once
#include <levk/asset_store.hpp>
#include <levk/core/time.hpp>
#include <levk/logger.hpp>
#include <levk/materials/unlit.hpp>
#include <levk/pipeline.hpp>
#include <levk/render_camera.hpp>
#include <levk/render_device.hpp>
#include <levk/render_object.hpp>
#include <levk/render_stats.hpp>
#include <levk/render_view.hpp>
#include <levk/shader_buffer.hpp>
#include <optional>
#include <vector>

namespace levk {
class ShaderContext {
  public:
	template <typename Type>
	struct Data {
		struct {
			Type skybox{};
		} vertex{};

		struct {
			Type shadow{};
			Type skybox{};
		} fragment{};
	};

	using Uris = Data<std::string_view>;

	explicit ShaderContext(IAssetStore& asset_store, Uris uris);

	Data<RenderShader> data{};
};

/// \brief Renderer for RenderObjects.
class RenderContext {
  public:
	static constexpr auto fov_v = Degrees{60.0f};
	static constexpr auto z_plane_v = glm::vec2{0.1f, 100.0f};

	explicit RenderContext(NotNull<IRenderDevice*> render_device, vk::CommandBuffer command_buffer, ShaderContext shaders);

	[[nodiscard]] static auto create(NotNull<IRenderDevice*> render_device, ShaderContext shaders) -> std::optional<RenderContext>;

	[[nodiscard]] auto get_command_buffer() const -> vk::CommandBuffer { return m_command_buffer; }
	[[nodiscard]] auto get_last_render_target() const -> RenderTarget const& { return m_last_rt; }

	/// \brief Add objects to the RenderPass.
	/// Referenced data in all objects must remain alive until after render().
	void add_objects(std::span<RenderObject const> objects);
	void add_skybox(NotNull<ICubemap const*> cubemap);

	auto set_camera(NotNull<IRenderCamera*> camera) -> RenderContext&;
	auto set_view(RenderView const& view) -> RenderContext&;

	auto draw_shadows(glm::ivec2 resolution, glm::vec3 const& projection_viewport) -> RenderStats;
	auto draw_renderers(glm::ivec2 resolution, Degrees field_of_view = fov_v, glm::vec2 z_plane = z_plane_v) -> RenderStats;

	/// \brief Clear all stored objects.
	void clear();

	void submit();

  private:
	enum class DrawType { eRenderers, eShadows };

	struct BakedObject {
		PrimitivesView primitives{};
		Ptr<IStorageBuffer const> instances{};
		std::uint32_t instance_count{};
		Ptr<IStorageBuffer const> joint_matrices{};
		std::optional<vk::PolygonMode> polygon_mode{};
		float line_width{};
		bool disable_depth_test{};
		std::optional<vk::CompareOp> depth_compare{};
		bool alpha_blend{};
	};

	struct Skybox {
		std::unique_ptr<material::Unlit> material{};
		std::unique_ptr<IStaticPrimitive> cube{};
		Ptr<ICubemap const> cubemap{};
		std::optional<NotNull<IPrimitive const*>> primitive{};
		RenderInstance instance{};
		std::optional<BakedObject> baked{};
	};

	auto bake(RenderObject const& object, BakedObject& out) -> bool;
	[[nodiscard]] auto get_pipeline(BakedObject const& object, IPrimitive const& primitive, DrawType type) const -> Ptr<IPipeline>;
	auto bind(IPipeline& pipeline) -> bool;
	auto draw(glm::ivec2 resolution, RenderBeginInfo const& info, DrawType type) -> RenderStats;
	void draw(IPipeline& pipeline, BakedObject const& object, IPrimitive const& primitive, DrawType type);

	Logger m_log{"RenderContext"};

	NotNull<IRenderDevice*> m_device;
	vk::CommandBuffer m_command_buffer{};
	ShaderContext m_shaders;

	Ptr<IRenderCamera> m_camera{};
	RenderView m_view{};
	RenderTarget m_last_rt{};
	Ptr<IRenderTexture const> m_shadow_map{};
	glm::mat4 m_shadow_view{};
	glm::mat4 m_shadow_proj{};

	Skybox m_skybox{};
	std::vector<BakedObject> m_objects{};

	std::vector<RenderInstance::Baked> m_instances{};

	struct {
		Ptr<IPipeline const> pipeline{};
		Ptr<IMaterial const> material{};
		Ptr<IStorageBuffer const> instances{};
		Ptr<IStorageBuffer const> joints{};
	} m_previous{};
};
} // namespace levk
