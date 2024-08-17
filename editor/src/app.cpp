#include <app.hpp>
#include <djson/json.hpp>
#include <levk/assets/asset_load_list.hpp>
#include <levk/assets/lit_material_asset.hpp>
#include <levk/assets/primitive_asset.hpp>
#include <levk/assets/scene_info_asset.hpp>
#include <levk/assets/shader_asset.hpp>
#include <levk/assets/texture_asset.hpp>
#include <levk/components/free_camera.hpp>
#include <levk/components/mesh_renderer.hpp>
#include <levk/components/orbit_camera.hpp>
#include <levk/components/primitive_renderer.hpp>
#include <levk/imcpp/im_controls.hpp>
#include <levk/imcpp/im_text.hpp>
#include <levk/materials/lit.hpp>
#include <levk/util.hpp>
#include <thread>

namespace {
template <typename T>
constexpr auto to_idx(T const in) {
	return static_cast<std::size_t>(in);
}
} // namespace

App::App(levk::NotNull<levk::VfsFiles const*> vfs)
	: m_vfs(vfs), m_engine(levk::create_engine()), m_asset_store(levk::create_asset_store(vfs, m_engine.get())) {}

void App::run(vk::SampleCountFlagBits const samples) {
	m_render_camera = m_engine->get_render_device().create_3d_camera(samples);
	m_render_camera->clear_colour = levk::colour::to_u8(levk::RgbaF32{0.5f, 0.5f, 0.5f, 1.0f});

	m_shadow_camera = m_engine->get_render_device().create_depth_only_camera(vk::SampleCountFlagBits::e1);

	m_executor_max_workers = static_cast<int>(std::thread::hardware_concurrency());

	m_camera_id = spawn_cameras();

	while (m_engine->is_window_open()) {
		m_engine->poll_events(*this);
		tick();
		render();
	}
}

void App::on_key_up(int const key, [[maybe_unused]] int const mods) {
	if (mods == 0) {
		switch (key) {
		case GLFW_KEY_ESCAPE: m_engine->shutdown(); break;
		case GLFW_KEY_I: m_engine->get_render_device().render_dear_imgui = !m_engine->get_render_device().render_dear_imgui; break;
		}
	}
}

void App::on_file_drop(std::span<char const*> paths) {
	for (auto const* path : paths) { handle_file_drop(path); }
}

void App::tick() {
	auto const dt = m_delta_time();
	tick(dt);
	inspect(dt);
}

void App::render() {
	auto render_context = levk::RenderContext::create(&m_engine->get_render_device());
	if (!render_context) { return; }

	render(*render_context);
}

void App::tick(levk::Seconds const dt) {
	if (m_loader.update()) { on_loaded(); }
	m_scene.tick(m_engine->get_input_state(), dt);
}

void App::render(levk::RenderContext& context) const {
	auto render_list = levk::RenderList{};
	m_scene.render_to(render_list);

	context.set_shadow_fragment_shader(*m_asset_store, "assets/shaders/noop.frag");
	context.set_skybox_vertex_shader(*m_asset_store, "assets/shaders/skybox.vert");
	context.set_skybox_fragment_shader(*m_asset_store, "assets/shaders/skybox.frag");

	context.add_objects(render_list.opaque);

	auto const render_view = m_scene.get_render_view(m_camera_id);
	context.set_view(render_view).set_camera(m_shadow_camera.get());
	context.draw_shadows({2048, 2048}, glm::vec3{100.0f});

	context.set_camera(m_render_camera.get());
	if (m_scene.skybox != nullptr) { context.add_skybox(m_scene.skybox); }
	m_render_stats = context.draw_renderers(m_engine->get_framebuffer_size(), m_fov, m_z_plane);

	context.submit();
}

auto App::spawn_cameras() -> levk::EntityId {
	auto& free_camera = m_scene.spawn_entity("free_camera");
	free_camera.attach_component(std::make_unique<levk::FreeCamera>());
	free_camera.transform.set_position({0.0f, 0.0f, 10.0f});
	m_cameras.free_cam = free_camera.get_id();

	auto& orbit_camera = m_scene.spawn_entity("orbit_camera");
	orbit_camera.attach_component(std::make_unique<levk::OrbitCamera>());
	m_cameras.orbit_cam = orbit_camera.get_id();

	return m_cameras.orbit_cam;
}

void App::on_loaded() {
	if (m_loader.asset_type == levk::StaticMeshAsset::type_name_v) {
		load_static_mesh(m_loader.asset_uri);
	} else if (m_loader.asset_type == levk::SkinnedMeshAsset::type_name_v) {
		load_skinned_mesh(m_loader.asset_uri);
	} else if (m_loader.asset_type == levk::SceneInfoAsset::type_name_v) {
		load_scene(m_loader.asset_uri);
	} else {
		m_log.warn("unknown asset: '{}'", m_loader.asset_uri);
	}

	auto const load_time = levk::Seconds{levk::Clock::now() - m_loader.start};
	m_log.info("loaded '{}' in {:.3f}s using {} threads", m_loader.asset_uri, load_time.count(), m_loader.workers);

	m_loader = {};
}

void App::handle_file_drop(levk::CString const path) {
	auto const potential_uri = m_vfs->to_uri(path.as_view());
	if (!potential_uri.empty()) { return handle_asset_drop(potential_uri); }

	auto fs_path = fs::path{path.as_view()};
	auto const extension = fs_path.extension();
	if (extension == ".gltf") { return handle_gltf_drop(fs_path); }

	m_log.warn("unrecognized file drop: '{}'", path.as_view());
}

void App::handle_asset_drop(std::string_view const uri) {
	if (m_loader.executor) {
		m_log.warn("ignoring request to load '{}', already loading: '{}'", uri, m_loader.asset_uri);
		return;
	}

	m_loader.asset_uri = std::string{uri};
	auto asset_list = levk::AssetLoadList{m_asset_store.get()};
	asset_list.add_asset(m_loader.asset_uri, &m_loader.asset_type);
	m_loader.workers = m_executor_workers;
	m_loader.executor = asset_list.execute_loads(m_loader.workers);
	if (!m_loader.executor) {
		m_log.warn("unknown asset / nothing to load: '{}'", uri);
		m_loader = {};
		return;
	}
	m_loader.start = levk::Clock::now();
}

void App::handle_gltf_drop(fs::path const& path) {
	auto const importer_shader = levk::Importer::Shader{
		.lit_vertex_uri = "assets/shaders/lit.vert",
		.skinned_vertex_uri = "assets/shaders/skin.vert",
		.tbn_vertex_uri = "assets/shaders/tbn.vert",
		.lit_fragment_uri = "assets/shaders/lit.frag",
		.tbn_fragment_uri = "assets/shaders/tbn.frag",
	};
	try {
		auto importer_builder = levk::Importer::Builder{m_vfs->get_mount_point(), importer_shader};
		importer_builder.import_root_dir = "assets";
		importer_builder.force_import = m_force_import;
		auto importer = importer_builder.build(path.string().c_str());
		auto const& import_asset = importer.get_import_asset();
		if (import_asset.scenes.empty() || import_asset.nodes.empty()) {
			m_log.warn("no scenes to load in GLTF: '{}'", path.generic_string());
			return;
		}
		auto const& import_scene = import_asset.scenes.front();
		auto const scene_uri = importer.import_scene(import_scene.index);
		if (scene_uri.empty()) {
			m_log.warn("failed to import GLTF scene '{}'", levk::to_size_t(import_scene.index));
			return;
		}
		handle_asset_drop(scene_uri);

	} catch (std::exception const& e) { m_log.error("PANIC: {}", e.what()); }
}

template <typename AssetT>
auto App::try_load_mesh(std::string_view const uri) -> levk::Ptr<typename AssetT::MeshType> {
	auto* asset = m_asset_store->load<AssetT>(uri);
	if (asset == nullptr) { return {}; }
	return &asset->mesh;
}

void App::load_scene(std::string_view const uri) {
	auto const* asset = m_asset_store->load<levk::SceneInfoAsset>(uri);
	if (asset == nullptr) { return; }

	levk::tree_visit(asset->scene, [&](levk::ImportedNode const& in_node) {
		auto const parent = [&in_node, asset] {
			auto const* parent = asset->scene.get_node(in_node.get_parent_id());
			return parent != nullptr ? parent->get_import_index() : levk::ImportIndex::eNone;
		}();
		load_node(in_node, parent);
	});

	if (!asset->scene.skybox.empty()) { load_skybox(asset->scene.skybox); }
	if (asset->scene.main_light) { m_scene.main_light = *asset->scene.main_light; }
}

void App::load_node(levk::ImportedNode const& node, levk::ImportIndex const parent) {
	if (node.camera && node.camera->type == levk::ImportedCamera::Type::ePerspective) {
		m_z_plane.x = node.camera->z_near;
		if (node.camera->z_far) { m_z_plane.y = *node.camera->z_far; }
		if (node.camera->y_fov) { m_fov = *node.camera->y_fov; }

		auto* entity = m_scene.get_entity(m_camera_id);
		auto* camera = entity != nullptr ? entity->get_component<levk::FreeCamera>() : nullptr;
		if (camera != nullptr) {
			entity->transform.set_position(node.transform.get_position());
			auto const euler = glm::eulerAngles(node.transform.get_orientation());
			camera->pitch = levk::Radians{euler.x};
			camera->yaw = levk::Radians{euler.y};
		}
		return;
	}

	auto& entity = m_scene.spawn_entity(node.name, node.get_import_index());
	entity.transform = node.transform;
	if (auto* parent_entity = m_scene.find_node_by_index(parent)) { m_scene.set_parent(entity, *parent_entity); }
	if (!node.mesh.empty()) {
		if (!node.skeleton.empty()) {
			attach_skinned_mesh(entity, node.mesh);
		} else {
			attach_static_mesh(entity, node.mesh);
		}
	}
}

void App::load_node_and_children(levk::ImportedScene const& scene, levk::ImportedNode const& node, levk::ImportIndex parent) {
	auto const* parent_node = scene.find_node_by_index(parent);
	auto const parent_index = parent_node != nullptr ? parent_node->get_import_index() : levk::ImportIndex::eNone;
	load_node(node, parent_index);
	for (auto const child_index : node.get_children_ids()) { load_node_and_children(scene, *scene.get_node(child_index), node.get_import_index()); }
}

auto App::load_static_mesh(std::string_view const uri) -> bool {
	auto& entity = m_scene.spawn_entity(std::format("mesh_{}", fs::path{uri}.stem().string()));
	if (!attach_static_mesh(entity, uri)) {
		entity.set_destroyed();
		return false;
	}
	return true;
}

auto App::load_skinned_mesh(std::string_view const uri) -> bool {
	auto& entity = m_scene.spawn_entity(std::format("mesh_{}", fs::path{uri}.stem().string()));
	if (!attach_skinned_mesh(entity, uri)) {
		entity.set_destroyed();
		return false;
	}
	return true;
}

auto App::load_skybox(std::string_view const uri) -> bool {
	auto const* cubemap = m_asset_store->load<levk::CubemapAsset>(uri);
	if (cubemap == nullptr) {
		m_log.error("failed to load Cubemap: '{}'", uri);
		return false;
	}

	m_scene.skybox = cubemap->cubemap.get();
	return true;
}

auto App::attach_static_mesh(levk::Entity& out, std::string_view const uri) -> bool {
	auto const* mesh = try_load_mesh<levk::StaticMeshAsset>(uri);
	if (mesh == nullptr) {
		m_log.error("failed to load StaticMesh: '{}'", uri);
		return false;
	}

	out.attach_component(std::make_unique<levk::StaticMeshRenderer>(mesh));
	return true;
}

auto App::attach_skinned_mesh(levk::Entity& out, std::string_view const uri) -> bool {
	auto const* mesh = try_load_mesh<levk::SkinnedMeshAsset>(uri);
	if (mesh == nullptr) {
		m_log.error("failed to load SkinnedMesh: '{}'", uri);
		return false;
	}

	out.attach_component(std::make_unique<levk::SkinnedMeshRenderer>(mesh));
	return true;
}

void App::inspect(levk::Seconds const dt) {
	draw_general_window(dt);
	draw_inspector_window();
	draw_debug_window();
}

void App::draw_general_window(levk::Seconds const dt) {
	ImGui::SetNextWindowSize({300.0f, 400.0f}, ImGuiCond_Once);
	if (ImGui::Begin("General")) {
		if (ImGui::BeginTabBar("GeneralTabs")) {
			draw_general_tabs(dt);
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void App::draw_general_tabs(levk::Seconds const dt) {
	draw_scene_tab();
	draw_assets_tab(dt);
}

void App::draw_scene_tab() {
	if (ImGui::BeginTabItem("Scene")) {
		m_scene.im_tree(m_inspect_target);
		ImGui::EndTabItem();
	}
}

void App::draw_assets_tab(levk::Seconds const dt) {
	if (ImGui::BeginTabItem("Assets")) {
		ImGui::SetNextItemWidth(150.0f);
		ImGui::SliderInt("load threads", &m_executor_workers, 1, m_executor_max_workers);
		ImGui::Checkbox("force import", &m_force_import);
		if (ImGui::Button("clear assets and scene")) {
			m_scene.clear_nodes();
			m_asset_store->clear();
			m_camera_id = spawn_cameras();
		}

		ImGui::Separator();
		m_asset_tree.draw(*m_asset_store, dt);
		ImGui::EndTabItem();
	}
}

void App::draw_inspector_window() {
	if (m_inspect_target == levk::EntityId::eNone) { return; }
	auto* entity = m_scene.get_entity(m_inspect_target);
	if (entity == nullptr) {
		m_inspect_target = levk::EntityId::eNone;
		return;
	}

	ImGui::SetNextWindowSize({400.0f, 400.0f}, ImGuiCond_Once);
	auto show_inspector = true;
	if (ImGui::Begin("Inspector", &show_inspector)) { entity->im_inspect(); }
	ImGui::End();
	if (!show_inspector) { m_inspect_target = levk::EntityId::eNone; }
}

void App::draw_debug_window() {
	if (ImGui::Begin("Debug")) {
		if (ImGui::BeginCombo("camera", m_camera_id == m_cameras.free_cam ? "free" : "orbit")) {
			if (ImGui::Selectable("free", m_camera_id == m_cameras.free_cam)) { m_camera_id = m_cameras.free_cam; }
			if (ImGui::Selectable("orbit", m_camera_id == m_cameras.orbit_cam)) { m_camera_id = m_cameras.orbit_cam; }
			ImGui::EndCombo();
		}
		ImGui::Checkbox("unified scale", &levk::Transform::im_unified_scale);

		ImGui::Separator();
		levk::im_text("frame time: {:.2f}", m_delta_time.value.count() * 1000.0f);
		levk::im_text("render time: {:.2f}", m_render_stats.render_time.count() * 1000.0f);
		levk::im_text("draw calls: {}", m_render_stats.draw_calls);
		levk::im_text("triangles: {}", m_render_stats.triangles);
		levk::im_text("pipelines: {}", m_render_stats.pipelines);
		levk::im_text("binds: {}", m_render_stats.pipeline_binds);
		auto wireframe = m_render_camera->polygon_mode == vk::PolygonMode::eLine;
		if (ImGui::Checkbox("wireframe", &wireframe)) { m_render_camera->polygon_mode = wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill; }
	}
	ImGui::End();
}
