#pragma once
#include <levk/core/c_string.hpp>
#include <levk/core/not_null.hpp>
#include <levk/core/time.hpp>
#include <levk/input/state.hpp>
#include <levk/lights.hpp>
#include <levk/logger.hpp>
#include <levk/node_tree.hpp>
#include <levk/render_list.hpp>
#include <levk/render_view.hpp>
#include <ranges>
#include <typeindex>
#include <unordered_set>

namespace levk {
class Scene;
class Entity;

using EntityId = TreeNodeId;

class IComponent : public Polymorphic {
  public:
	virtual void setup([[maybe_unused]] Entity& entity) {}

	[[nodiscard]] virtual auto get_im_label() const -> CString { return {}; }
	virtual void im_inspect() {}
};

class ITickComponent : public IComponent {
  public:
	virtual void tick(Entity& entity, Seconds dt) = 0;
};

class IRenderComponent : public ITickComponent {
  public:
	virtual void render_to(Entity const& entity, RenderList& render_list) const = 0;

	[[nodiscard]] auto get_render_object(Entity const& entity, PrimitivesView primitives, std::span<glm::mat4 const> joint_matrices) const -> RenderObject;

	std::vector<RenderInstance> instances{};
	std::optional<vk::PolygonMode> polygon_mode{};
	float line_width{1.0f};
};

class ISceneCamera : public ITickComponent {
  public:
	void setup(Entity& entity) override;
	void im_inspect() override;

	float exposure{3.0f};
};

class Entity : public TreeNode {
  public:
	using Id = EntityId;

	[[nodiscard]] auto get_scene() const -> Scene& { return *m_scene; }

	[[nodiscard]] auto get_parent() const -> Ptr<Entity>;
	void set_parent(Entity const& parent);
	void unparent();

	[[nodiscard]] auto get_model_matrix() const -> glm::mat4;
	[[nodiscard]] auto get_world_position() const -> glm::vec3 { return glm::vec3{get_model_matrix()[3]}; }

	template <typename Type>
	void attach_component(std::unique_ptr<Type> component) {
		if (!component) { return; }
		if constexpr (std::derived_from<Type, IRenderComponent>) {
			m_render_components.push_back(component.get());
			m_tick_components.push_back(component.get());
		} else if constexpr (std::derived_from<Type, ITickComponent>) {
			m_tick_components.push_back(component.get());
		}
		component->setup(*this);
		m_components.insert_or_assign(typeid(Type), std::move(component));
	}

	template <typename Type>
	[[nodiscard]] auto get_component() const -> Ptr<Type> {
		if (auto const it = m_components.find(typeid(Type)); it != m_components.end()) { return static_cast<Type*>(it->second.get()); }
		for (auto const& [_, component] : m_components) {
			if (auto* ret = dynamic_cast<Type*>(component.get())) { return ret; }
		}
		return nullptr;
	}

	[[nodiscard]] auto is_visible() const -> bool;

	[[nodiscard]] auto is_destroyed() const -> bool { return m_destroyed; }
	void set_destroyed() { m_destroyed = true; }

	void tick(Seconds dt);
	void render_to(RenderList& render_list) const;

	[[nodiscard]] auto get_input_state() const -> input::State const&;

	void im_inspect();

	bool visible{true};

  private:
	explicit Entity(NotNull<Scene*> scene) : m_scene(scene) {}

	NotNull<Scene*> m_scene;

	std::unordered_map<std::type_index, std::unique_ptr<IComponent>> m_components{};
	std::vector<NotNull<ITickComponent*>> m_tick_components{};
	std::vector<NotNull<IRenderComponent*>> m_render_components{};

	std::vector<NotNull<ITickComponent*>> m_tick_cache{};

	bool m_destroyed{};

	friend class Scene;
};

class Scene : public BasicNodeTree<Entity> {
  public:
	using RenderSkip = std::unordered_set<EntityId>;

	Scene() = default;

	explicit Scene(std::string tag) : m_log(std::move(tag)) {}

	[[nodiscard]] auto spawn_entity(std::string name, ImportIndex import_index = ImportIndex::eNone) -> Entity&;

	[[nodiscard]] auto get_entity(this auto&& self, Id const id) { return self.get_node(id); }

	[[nodiscard]] auto get_scene_camera(Id entity_id) const -> Ptr<ISceneCamera>;
	[[nodiscard]] auto get_render_view(Id camera_entity) const -> RenderView;

	[[nodiscard]] auto get_camera_ids() const -> std::span<Id const> { return m_cameras; }

	[[nodiscard]] auto get_input_state() const -> input::State const&;

	void tick(input::State const& input_state, Seconds dt);
	void render_to(RenderList& render_list, RenderSkip const& skip = {}) const;

	void im_tree(Id& out_inspect_target);

	DirectionalLight main_light{};
	Ptr<ICubemap const> skybox{};

  protected:
	Logger m_log{"Scene"};

  private:
	NodeCache m_tick_cache{};

	std::vector<Id> m_cameras{};

	Ptr<input::State const> m_input_state{};

	friend class ISceneCamera;
};
} // namespace levk
