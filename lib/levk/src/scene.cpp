#include <levk/imcpp/im_controls.hpp>
#include <levk/imcpp/im_text.hpp>
#include <levk/scene.hpp>

namespace levk {
auto IRenderComponent::get_render_object(Entity const& entity, PrimitivesView const primitives,
										 std::span<glm::mat4 const> joint_matrices) const -> RenderObject {
	return RenderObject{
		.primitives = primitives,
		.instances = instances.empty() ? std::span{&render_instance_v, 1} : instances,
		.parent = entity.get_model_matrix(),
		.joint_matrices = joint_matrices,
		.polygon_mode = polygon_mode,
		.line_width = line_width,
	};
}

void ISceneCamera::setup(Entity& entity) {
	IComponent::setup(entity);
	entity.get_scene().m_cameras.push_back(entity.get_id());
}

void ISceneCamera::im_inspect() { ImDragFloat{.range = {0.5f, 500.0f}}.f32("exposure", exposure, 1.0f); }

auto Entity::get_parent() const -> Ptr<Entity> { return m_scene->get_node(get_parent_id()); }

void Entity::set_parent(Entity const& parent) { m_scene->set_parent(*this, parent.get_id()); }

void Entity::unparent() { m_scene->unparent(*this); }

auto Entity::get_model_matrix() const -> glm::mat4 { return get_scene().get_model_matrix(*this); }

auto Entity::is_visible() const -> bool {
	if (!visible) { return false; }
	if (auto const* parent = get_parent()) { return parent->is_visible(); }
	return true;
}

void Entity::tick(Seconds const dt) {
	m_tick_cache.clear();
	m_tick_cache.insert(m_tick_cache.end(), m_tick_components.begin(), m_tick_components.end());
	for (auto component : m_tick_cache) { component->tick(*this, dt); }
}

void Entity::render_to(RenderList& render_list) const {
	for (auto const component : m_render_components) { component->render_to(*this, render_list); }
}

auto Entity::get_input_state() const -> input::State const& { return m_scene->get_input_state(); }

void Entity::im_inspect() {
	ImGui::SeparatorText(levk::FixedString{"{}", name}.c_str());

	ImGui::Separator();
	levk::im_text("id: {}", static_cast<int>(get_id()));
	ImGui::Checkbox("visible", &visible);
	if (ImGui::Button("destroy")) {
		set_destroyed();
		return;
	}

	static constexpr int framed_flags_v = ImGuiTreeNodeFlags_Framed;
	if (ImGui::TreeNodeEx("Transform", framed_flags_v | ImGuiTreeNodeFlags_DefaultOpen)) {
		transform.im_inspect();
		ImGui::TreePop();
	}

	for (auto const& [_, component] : m_components) {
		auto const label = component->get_im_label();
		if (label.as_view().empty()) { continue; }
		if (ImGui::TreeNodeEx(label.c_str(), framed_flags_v)) {
			component->im_inspect();
			ImGui::TreePop();
		}
	}
}

auto Scene::spawn_entity(std::string name, ImportIndex const import_index) -> Entity& {
	auto entity = Entity{this};
	entity.name = std::move(name);
	return add_node(std::move(entity), import_index);
}

auto Scene::get_scene_camera(Id const entity_id) const -> Ptr<ISceneCamera> {
	auto const* entity = get_entity(entity_id);
	if (entity == nullptr) { return nullptr; }
	return entity->get_component<ISceneCamera>();
}

auto Scene::get_render_view(Id const camera_entity) const -> RenderView {
	auto const* entity = get_entity(camera_entity);
	if (entity == nullptr) { return {}; }
	auto const* scene_camera = entity->get_component<ISceneCamera>();
	if (scene_camera == nullptr) { return {}; }

	return RenderView{
		.camera_transform = get_model_matrix(*entity),
		.camera_exposure = scene_camera->exposure,
		.main_light = main_light,
	};
}

auto Scene::get_input_state() const -> input::State const& {
	if (m_input_state != nullptr) { return *m_input_state; }
	static auto const default_v{input::State{}};
	return default_v;
}

void Scene::tick(input::State const& input_state, Seconds dt) {
	m_input_state = &input_state;
	m_tick_cache.clear();
	fill_nodes(m_tick_cache);
	for (auto const entity : m_tick_cache) { entity->tick(dt); }
	remove_if([](Entity const& e) { return e.is_destroyed(); });
	m_input_state = {};
	std::erase_if(m_cameras, [this](Id const id) { return this->get_entity(id) == nullptr; });
}

void Scene::render_to(RenderList& render_list, RenderSkip const& skip) const {
	struct WrapIsVisible {
		[[nodiscard]] static auto begin_visit(Entity const& e) { return e.visible; }
		[[nodiscard]] static auto end_visit(Entity const& /*e*/) {}
	};

	auto const visitor = [&](Entity const& e) {
		if (skip.contains(e.get_id())) { return; }
		e.render_to(render_list);
	};

	tree_visit_wrap(WrapIsVisible{}, *this, visitor);
}

void Scene::im_tree(Id& out_inspect_target) {
	if (ImGui::TreeNode("main light")) {
		main_light.inspect_directional_light();
		ImGui::TreePop();
	}

	ImGui::Separator();

	struct WrapImTreeNode {
		Id& inspect_target; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

		auto begin_visit(levk::Entity const& entity) -> bool {
			auto const label = levk::FixedString<>{"[{}] {}", static_cast<int>(entity.get_id()), entity.name};
			int flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
			if (inspect_target == entity.get_id()) { flags |= ImGuiTreeNodeFlags_Selected; }
			if (entity.get_children_ids().empty()) { flags |= ImGuiTreeNodeFlags_Leaf; }
			auto const ret = ImGui::TreeNodeEx(label.c_str(), flags);
			if (!ImGui::IsItemToggledOpen() && ImGui::IsItemClicked(ImGuiMouseButton_Left)) { inspect_target = entity.get_id(); }
			return ret;
		}

		static void end_visit(levk::Entity const& /*entity*/) { ImGui::TreePop(); }
	};

	static constexpr int flags_v = ImGuiWindowFlags_HorizontalScrollbar;
	ImGui::BeginChild("Scene", {-1.0f, -1.0f}, true, flags_v);
	levk::tree_visit_wrap(WrapImTreeNode{.inspect_target = out_inspect_target}, *this);
	ImGui::EndChild();
}
} // namespace levk
