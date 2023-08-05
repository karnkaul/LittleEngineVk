#pragma once
#include <le/graphics/camera.hpp>
#include <le/graphics/lights.hpp>
#include <le/node/node_tree.hpp>
#include <le/scene/collision.hpp>
#include <le/scene/entity.hpp>
#include <le/scene/ui/view.hpp>

namespace le {
class Scene {
  public:
	Scene(Scene const&) = delete;
	Scene(Scene&&) = delete;
	auto operator=(Scene const&) -> Scene& = delete;
	auto operator=(Scene&&) -> Scene& = delete;

	Scene() = default;
	virtual ~Scene();

	auto spawn(std::string name = "unnamed", Ptr<Entity> parent = {}) -> Entity&;

	[[nodiscard]] auto has_entity(Id<Entity> id) const -> bool { return find_entity(id) != nullptr; }

	[[nodiscard]] auto find_entity(Id<Entity> id) const -> Ptr<Entity const>;
	[[nodiscard]] auto find_entity(Id<Entity> id) -> Ptr<Entity>;

	[[nodiscard]] auto get_entity(Id<Entity> id) const noexcept(false) -> Entity const&;
	[[nodiscard]] auto get_entity(Id<Entity> id) noexcept(false) -> Entity&;

	[[nodiscard]] auto get_node_tree() const -> NodeTree const& { return m_node_tree; }
	[[nodiscard]] auto make_node_locator() -> NodeLocator { return m_node_tree; }

	[[nodiscard]] auto get_ui_root() const -> ui::View const& { return m_ui_root; }
	[[nodiscard]] auto get_ui_root() -> ui::View& { return m_ui_root; }

	auto reparent(Entity& entity, Entity& parent) -> void;
	auto unparent(Entity& entity) -> void;

	[[nodiscard]] auto entity_count() const -> std::size_t { return m_entity_map.size(); }
	auto clear_entities() -> void;

	virtual auto setup() -> void;
	virtual auto tick(Duration dt) -> void;
	virtual auto render_entities(std::vector<graphics::RenderObject>& out) const -> void;

	graphics::Lights lights{};
	graphics::Camera main_camera{};
	graphics::Camera ui_camera{.type = graphics::Camera::Orthographic{}};
	Ptr<graphics::Cubemap const> skybox{};

	Collision collision{};

  private:
	std::unordered_map<Id<Entity>::id_type, Entity> m_entity_map{};
	NodeTree m_node_tree{};
	ui::View m_ui_root{};
	Id<Entity>::id_type m_next_id{};

	struct {
		std::vector<Ptr<Entity>> entities{};
		mutable std::vector<NotNull<RenderComponent const*>> render_components{};
	} m_active{};
	std::vector<Id<Entity>> m_destroyed{};
};
} // namespace le
