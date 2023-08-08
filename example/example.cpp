#include <imgui.h>
#include <le/core/fixed_string.hpp>
#include <le/core/logger.hpp>
#include <le/imcpp/engine_stats.hpp>
#include <le/importer/importer.hpp>
#include <le/input/receiver.hpp>
#include <le/resources/mesh_asset.hpp>
#include <le/resources/pcm_asset.hpp>
#include <le/scene/freecam_controller.hpp>
#include <le/scene/imcpp/scene_graph.hpp>
#include <le/scene/mesh_animator.hpp>
#include <le/scene/mesh_renderer.hpp>
#include <le/scene/runtime.hpp>
#include <le/scene/ui/text.hpp>
#include <le/vfs/file_reader.hpp>
#include <le/vfs/vfs.hpp>

namespace example {
using namespace le;

namespace {
auto const g_log = le::logger::Logger{"Example"};

// using Scene requires linking to le::le-scene.
class BrainStem : public Scene {
  public:
	// store imported mesh Uri to later load and render.
	BrainStem(Uri mesh_uri) : m_mesh_uri(std::move(mesh_uri)) {}

  private:
	auto setup() -> void final {
		Scene::setup();

		setup_camera();

		spawn_brain_stem();
		spawn_free_cam();
		set_title_text();
	}

	auto tick(Duration dt) -> void final {
		Scene::tick(dt);

		if (Resources::is_ready(m_mesh_future)) {
			if (auto const* mesh_asset = m_mesh_future.get()) { spawn_mesh(&mesh_asset->mesh, m_loading_uri); }
			m_loading_uri = {};
		}

		if (Resources::is_ready(m_pcm_future)) {
			if (auto const* pcm = m_pcm_future.get()) {
				auto& audio_device = audio::Device::self();
				audio_device.set_track(&pcm->pcm);
				audio_device.play_music();
			}
			m_loading_uri = {};
		}

		for (auto const& drop : Engine::self().input_state().drops) { handle_drop(drop); }

		update_editor();

		// check the current input state: this is fast but too coarse for say text input,
		// which should use input::Receiver (like 'App' does below, or 'ui::InputText').
		if (Engine::self().input_state().keyboard[GLFW_KEY_R] == input::Action::eRelease) {
			// reload this scene (or load some oher scene).
			// the actual switching is deferred to the next tick, so 'this' is still valid.
			SceneSwitcher::self().switch_scene(std::make_unique<BrainStem>(m_mesh_uri));
		}
	}

	auto setup_camera() -> void {
		// offset the camera to roughly centre the expected mesh.
		main_camera.transform.set_position({0.0f, 1.0f, 3.0f});
		// set a non-black clear colour.
		main_camera.clear_colour = graphics::Rgba{.channels = {0x20, 0x15, 0x10, 0xff}};
		// reduce shadow frustum to increase sharpness. (small scene.)
		graphics::Renderer::self().shadow_frustum = glm::vec3{15.0f};
	}

	auto spawn_brain_stem() -> void {
		// load mesh and its assets (materials, textures, skeleton, etc).
		// load_async() can be used to obtain a std::future.
		auto const* mesh_asset = Resources::self().load<MeshAsset>(m_mesh_uri);
		// hard exit if the mesh failed to load, for the purpose of this example.
		if (mesh_asset == nullptr) { throw Error{std::format("failed to load MeshAsset [{}]", m_mesh_uri.value())}; }

		m_mesh_entity = spawn_mesh(&mesh_asset->mesh, m_mesh_uri.value());
		auto& mesh_entity = get_entity(*m_mesh_entity);

		// create a child entity to attach an offset AABB collider.
		auto& collider_entity = spawn("collider", &mesh_entity);
		auto& collider = collider_entity.attach(std::make_unique<ColliderAabb>());
		// offset the collider.
		collider.aabb_size = {0.5f, 1.5f, 0.5f};
		collider_entity.get_transform().set_position({0.0f, 0.5f * collider.aabb_size.y, 0.0f});
	}

	auto spawn_free_cam() -> void {
		// spawn a FreeCam entity.
		// hold right click to engage.
		auto& free_cam = spawn("freecam");
		m_freecam_entity = free_cam.id();
		free_cam.attach(std::make_unique<FreecamController>());
	}

	auto set_title_text() -> void {
		// setup some UI title text.
		ui::Text::default_font_uri = font_uri;
		auto title_text = std::make_unique<ui::Text>();
		title_text->set_text("BrainStem");
		// anchor to top of parent view (the whole screen, as it will be pushed to the root view).
		title_text->transform.anchor.y = 0.5f;
		// offset y downwards by 100 pixels.
		title_text->transform.position.y = -100.0f;
		// push title to root view.
		get_ui_root().push_sub_view(std::move(title_text));
	}

	auto update_editor() -> void {
		ImGui::SetNextWindowSize({300.0f, 200.0f}, ImGuiCond_Once);
		ImGui::SetNextWindowPos({100.0f, 100.0f}, ImGuiCond_Once);
		if (auto w = imcpp::Window{"editor"}) {
			ImGui::Checkbox("Engine Stats", &m_show_stats);
			ImGui::Separator();
			m_scene_graph.draw_to(w, *this);
		}

		if (m_show_stats) {
			ImGui::SetNextWindowSize({300.0f, 200.0f}, ImGuiCond_Once);
			ImGui::SetNextWindowPos({100.0f, 400.0f}, ImGuiCond_Once);
			if (auto w = imcpp::Window{"Engine Stats", &m_show_stats}) { m_engine_stats.draw_to(w); }
		}

		if (auto loading = imcpp::Modal{"loading"}) {
			ImGui::Text("%s", FixedString{"loading [{}]...", m_loading_uri.value()}.c_str());
			if (m_loading_uri.is_empty()) { imcpp::Modal::close_current(); }
		}
	}

	auto spawn_mesh(NotNull<graphics::Mesh const*> mesh, std::string_view const name) -> Id<Entity> {
		// spawn an entity and attach a mesh renderer (and animator).
		auto& mesh_entity = spawn(std::format("mesh_{}", name));
		auto ret = mesh_entity.id();
		mesh_entity.attach(std::make_unique<MeshRenderer>()).set_mesh(mesh);
		// allow users to "override" the GLTF data with a static mesh.
		if (mesh->skeleton != nullptr) { mesh_entity.attach(std::make_unique<MeshAnimator>()).set_skeleton(mesh->skeleton); }

		return ret;
	}

	auto handle_drop(std::string_view const path) -> void {
		if (!m_loading_uri.is_empty()) { return; }

		auto uri = dynamic_cast<FileReader const&>(vfs::get_reader()).to_uri(path);
		if (uri.is_empty()) { return; }

		auto const extension = uri.extension();

		if (extension == ".mp3" || extension == ".ogg" || extension == ".wav") {
			m_pcm_future = Resources::self().load_async<PcmAsset>(uri);
			m_loading_uri = std::move(uri);
			imcpp::Modal::open("loading");
			return;
		}

		if (extension == ".json" && Asset::get_asset_type(uri) == MeshAsset::type_name_v) {
			m_mesh_future = Resources::self().load_async<MeshAsset>(uri);
			m_loading_uri = std::move(uri);
			imcpp::Modal::open("loading");
			return;
		}
	}

	// hard coded Uri, unpacked from data.zip at CMake configure time.
	inline static Uri const font_uri{"font.ttf"};

	Uri m_mesh_uri{};
	std::optional<EntityId> m_mesh_entity{};
	std::optional<EntityId> m_freecam_entity{};

	std::future<Ptr<MeshAsset>> m_mesh_future{};
	std::future<Ptr<PcmAsset>> m_pcm_future{};
	Uri m_loading_uri{};

	imcpp::SceneGraph m_scene_graph{};
	imcpp::EngineStats m_engine_stats{};
	bool m_show_stats{};
};

// 'input::Receiver's allow fine-grained reaction to keyboard and mouse input events.
// it adds 'this' to a stack in its constructor, which is walked backwards during event dispatch.
// a receiver that returns 'true' in its callbacks blocks the stack from being walked further.
// using Runtime requires linking to le::le-scene.
class App : public Runtime, public input::Receiver {
  public:
	explicit App(std::string_view const exe_path) {
		auto data_path = le::FileReader::find_super_directory("data/", exe_path);
		if (data_path.empty()) { throw Error{"failed to locate data"}; }

		g_log.info("mounting data at [{}]", data_path);

		le::vfs::set_reader(std::make_unique<le::FileReader>(data_path));

		// import GLTF into engine Mesh.
		// this would usually be done offline, and using le-importer (exe) directly.
		// using Importer requires linking to le::le-importer-lib.
		auto importer = importer::Importer{};
		auto input = importer::Input{};
		input.data_root = std::move(data_path);
		input.gltf_path = input.data_root / gltf_uri.value();
		input.force = true;
		if (!importer.setup(std::move(input))) { throw Error{"failed to setup le-importer"}; }

		auto uri = importer.import_mesh(0);
		// hard exit if the mesh failed to load, for the purpose of this example.
		if (!uri) { throw Error{std::format("failed to import mesh from [{}]", gltf_uri.value())}; }

		m_mesh_uri = std::move(*uri);
	}

  private:
	auto setup() -> void final {
		Runtime::setup();

		SceneSwitcher::self().switch_scene(std::make_unique<BrainStem>(m_mesh_uri));
	}

	// input::Receiver callback
	auto on_key(int key, int action, int mods) -> bool final {
		if (action == GLFW_RELEASE && key == GLFW_KEY_W && mods == GLFW_MOD_CONTROL) { Engine::self().shutdown(); }
		if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE && mods == 0) { Engine::self().shutdown(); }
		// this is the only receiver on the stack in this example so it doesn't matter,
		// but we wouldn't want to block input from going further down the stack if it wasn't.
		// a 'ui::InputText' instance in comparison WILL block if it's enabled.
		return false;
	}

	// hard coded Uri, unpacked from data.zip at CMake configure time.
	inline static Uri const gltf_uri{"BrainStem.gltf"};

	Uri m_mesh_uri{};
};
} // namespace
} // namespace example

auto main(int argc, char** argv) -> int {
	if (argc < 1) { return EXIT_FAILURE; }

	try {
		example::App{*argv}.run();
	} catch (std::exception const& e) {
		example::g_log.error("fatal error: {}", e.what());
		return EXIT_FAILURE;
	}
}
