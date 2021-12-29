#include <core/not_null.hpp>
#include <core/utils/algo.hpp>
#include <core/utils/std_hash.hpp>
#include <core/utils/string.hpp>
#include <dumb_tasks/scheduler.hpp>
#include <engine/assets/asset_list.hpp>
#include <engine/cameras/freecam.hpp>
#include <engine/editor/inspector.hpp>
#include <engine/engine.hpp>
#include <engine/input/control.hpp>
#include <engine/render/model.hpp>
#include <engine/scene/scene_node.hpp>
#include <graphics/common.hpp>
#include <graphics/render/shader_buffer.hpp>
#include <graphics/utils/utils.hpp>
#include <fstream>

#include <engine/editor/editor.hpp>
#include <engine/editor/scene_tree.hpp>
#include <engine/gui/quad.hpp>
#include <engine/gui/text.hpp>
#include <engine/gui/view.hpp>
#include <engine/gui/widget.hpp>
#include <engine/render/list_renderer.hpp>
#include <engine/render/skybox.hpp>
#include <engine/scene/draw_list_gen.hpp>
#include <engine/scene/scene_registry.hpp>
#include <engine/utils/exec.hpp>

#include <core/utils/enumerate.hpp>
#include <engine/assets/asset_manifest.hpp>
#include <engine/gui/widgets/dropdown.hpp>
#include <engine/render/descriptor_helper.hpp>

#include <core/utils/shell.hpp>
#include <core/utils/tween.hpp>
#include <engine/gui/widgets/input_field.hpp>
#include <engine/input/text_cursor.hpp>
#include <engine/render/quad_emitter.hpp>
#include <engine/render/text_mesh.hpp>
#include <graphics/font/font.hpp>
#include <graphics/utils/instant_command.hpp>
#include <ktl/async/kasync.hpp>

#include <engine/ecs/components/spring_arm.hpp>
#include <engine/ecs/components/trigger.hpp>

namespace le::demo {
using RGBA = graphics::RGBA;

enum class Flag { eClosed, eDebug0 };
using Flags = ktl::enum_flags<Flag, u8>;

static void poll(Flags& out_flags, window::EventQueue const& queue) {
	for (auto const& event : queue) {
		switch (event.type) {
		case window::Event::Type::eClose: {
			out_flags.set(Flag::eClosed);
			break;
		}
		default: break;
		}
	}
}

using namespace dts;

using namespace std::chrono;

struct ViewMats {
	alignas(16) glm::mat4 mat_v;
	alignas(16) glm::mat4 mat_p;
	alignas(16) glm::mat4 mat_ui;
	alignas(16) glm::vec4 pos_v;
};

struct Albedo {
	alignas(16) glm::vec4 ambient;
	alignas(16) glm::vec4 diffuse;
	alignas(16) glm::vec4 specular;

	static Albedo make(Colour colour = colours::white, glm::vec4 const& amdispsh = {0.5f, 0.8f, 0.4f, 42.0f}) noexcept {
		return make(colour.toVec4(), amdispsh);
	}

	static Albedo make(glm::vec4 const& colour, glm::vec4 const& amdispsh = {0.5f, 0.8f, 0.4f, 42.0f}) noexcept {
		Albedo ret;
		glm::vec3 const& c = colour;
		f32 const& a = colour.w;
		ret.ambient = {c * amdispsh.x, a};
		ret.diffuse = {c * amdispsh.y, a};
		ret.specular = {c * amdispsh.z, amdispsh.w};
		return ret;
	}
};

struct ShadeMat {
	alignas(16) glm::vec4 tint;
	alignas(16) Albedo albedo;

	static ShadeMat make(Material const& mtl) noexcept {
		ShadeMat ret;
		ret.albedo.ambient = mtl.Ka.toVec4();
		ret.albedo.diffuse = mtl.Kd.toVec4();
		ret.albedo.specular = mtl.Ks.toVec4();
		ret.tint = {static_cast<glm::vec3 const&>(mtl.Tf.toVec4()), mtl.d};
		return ret;
	}
};

struct DirLight {
	alignas(16) Albedo albedo;
	alignas(16) glm::vec4 direction;
};

struct DirLights {
	alignas(16) std::array<DirLight, 4> lights;
	alignas(4) u32 count;
};

struct PlayerController {
	glm::quat target = graphics::identity;
	f32 roll = 0.0f;
	f32 maxRoll = glm::radians(45.0f);
	f32 speed = 0.0f;
	f32 maxSpeed = 10.0f;
	bool active = true;

	void tick(input::State const& state, Transform& transform, Time_s dt) noexcept {
		input::Range r(input::KeyRange{input::Key::eA, input::Key::eD});
		roll = r(state);
		if (maths::abs(roll) < 0.25f) {
			roll = 0.0f;
		} else if (maths::abs(roll) > 0.9f) {
			roll = roll < 0.0f ? -1.0f : 1.0f;
		}
		r = input::KeyRange{input::Key::eS, input::Key::eW};
		f32 dspeed = r(state);
		if (maths::abs(dspeed) < 0.25f) {
			dspeed = 0.0f;
		} else if (maths::abs(dspeed) > 0.9f) {
			dspeed = dspeed < 0.0f ? -1.0f : 1.0f;
		}
		speed = std::clamp(speed + dspeed * 0.03f, -maxSpeed, maxSpeed);
		glm::quat const orient = glm::rotate(graphics::identity, -roll * maxRoll, graphics::front);
		target = glm::slerp(target, orient, dt.count() * 10);
		transform.orient(target);
		glm::vec3 const dpos = roll * graphics::right + speed * (transform.orientation() * -graphics::front);
		transform.position(transform.position() + 10.0f * dpos * dt.count());
	}
};

class Renderer : public ListRenderer {
  public:
	using Camera = graphics::Camera;

	struct Scene {
		dens::registry const* registry{};
		Camera const* camera{};
		Span<DirLight const> lights;
		glm::vec2 size{};
	};

	Renderer(not_null<graphics::VRAM*> vram) : m_vram(vram) {
		m_view.mats = graphics::ShaderBuffer(vram, {});
		m_view.lights = graphics::ShaderBuffer(vram, {});
	}

	void render(RenderPass& out_rp, Scene const& scene) {
		m_view.lights.swap();
		m_view.mats.swap();
		if (scene.camera) {
			auto const& cam = *scene.camera;
			ViewMats const v{cam.view(), cam.perspective(scene.size), cam.ortho(scene.size), {cam.position, 1.0f}};
			m_view.mats.write(v);
		} else {
			m_view.mats.write(ViewMats());
		}
		if (!scene.lights.empty()) {
			DirLights dl;
			for (std::size_t idx = 0; idx < scene.lights.size() && idx < dl.lights.size(); ++idx) { dl.lights[idx] = scene.lights[idx]; }
			dl.count = std::min((u32)scene.lights.size(), (u32)dl.lights.size());
			m_view.lights.write(dl);
		} else {
			m_view.lights.write(DirLights());
		}
		DrawableMap map;
		if (scene.registry) { fill(map, *scene.registry); }
		ListRenderer::render(out_rp, map);
	}

  private:
	struct {
		graphics::ShaderBuffer mats;
		graphics::ShaderBuffer lights;
	} m_view;
	not_null<graphics::VRAM*> m_vram;

	void writeSets(DescriptorMap map, DrawList const& list) override {
		auto set0 = list.set(map, 0);
		set0.update(0, m_view.mats);
		set0.update(1, m_view.lights);
		for (Drawable const& drawable : list.drawables) {
			drawable.set(map, 1).update(0, drawable.model);
			DrawMesh const& drawMesh = drawable.mesh;
			for (MeshObj const& mesh : drawable.meshViews()) {
				if (mesh.primitive) {
					static Material const s_blank;
					Material const& mat = mesh.material ? *mesh.material : s_blank;
					auto set2 = drawMesh.set(map, 2);
					set2.update(0, mat.map_Kd);
					set2.update(1, mat.map_d);
					set2.update(2, mat.map_Ks, TextureFallback::eBlack);
					drawMesh.set(map, 3).update(0, ShadeMat::make(mat));
				}
			}
		}
	}
};

class TestView : public gui::View {
  public:
	TestView(not_null<gui::ViewStack*> parent, std::string name, Hash fontURI = gui::defaultFontURI) : gui::View(parent, std::move(name)) {
		m_canvas.size.value = {1280.0f, 720.0f};
		m_canvas.size.unit = gui::Unit::eAbsolute;
		auto& bg = push<gui::Quad>();
		bg.m_rect.size = {200.0f, 100.0f};
		bg.m_rect.anchor.norm = {-0.25f, 0.25f};
		bg.m_material.Tf = colours::cyan;
		auto& centre = bg.push<gui::Quad>();
		centre.m_rect.size = {50.0f, 50.0f};
		centre.m_material.Tf = colours::red;
		auto& dot = centre.push<gui::Quad>();
		dot.offset({30.0f, 20.0f});
		dot.m_material.Tf = Colour(0x333333ff);
		dot.m_rect.anchor.norm = {-0.5f, -0.5f};
		auto& topLeft = bg.push<gui::Quad>();
		topLeft.m_rect.anchor.norm = {-0.5f, 0.5f};
		topLeft.offset({25.0f, 25.0f}, {1.0f, -1.0f});
		topLeft.m_material.Tf = colours::magenta;
		auto& text = bg.push<gui::Text>(fontURI);
		text.set("click").height(60U);
		m_button = &push<gui::Button>(fontURI);
		m_button->m_rect.size = {200.0f, 100.0f};
		m_button->m_text->set("Button").height(40U);
		m_button->refresh();
		m_onClick = m_button->onClick();
		m_onClick += [this]() { setDestroyed(); };
	}

	TestView(TestView&&) = delete;
	TestView& operator=(TestView&&) = delete;

	gui::Button* m_button{};
	gui::Widget::OnClick::handle m_onClick;
};
} // namespace le::demo

namespace le::gui {
class Dialogue : public View {
  public:
	struct CreateInfo;
	struct ButtonInfo {
		glm::vec2 size = {150.0f, 40.0f};
		u32 textHeight = 25U;
		Hash style;
	};

	Dialogue(not_null<gui::ViewStack*> parent, std::string name, CreateInfo const& info);

	[[nodiscard]] Widget::OnClick::handle addButton(std::string text, Widget::OnClick::callback&& onClick);

  protected:
	void onUpdate(input::Frame const& frame) override;

	struct Header {
		Button* title{};
		Button* close{};
	};
	struct Footer {
		Widget* bg{};
		std::vector<Button*> buttons;
	};
	struct {
		glm::vec2 prev{};
		glm::vec2 start{};
		glm::vec2 delta{};
		bool moving = false;
	} m_originDelta;
	Header m_header;
	Button* m_content{};
	Footer m_footer;
	ButtonInfo m_buttonInfo;
	Button::OnClick::handle m_closeSignal;
	Hash m_fontURI;
};

struct Dialogue::CreateInfo {
	struct Content {
		std::string text;
		glm::vec2 size = {500.0f, 200.0f};
		u32 textHeight = 25U;
		Hash style;
	};
	struct Header {
		std::string text;
		f32 height = 50.0f;
		graphics::RGBA background = {0x999999ff, graphics::RGBA::Type::eAbsolute};
		u32 textHeight = 30U;
		Hash style;
	};
	struct Footer {
		f32 height = 60.0f;
		graphics::RGBA background = {0x999999ff, graphics::RGBA::Type::eAbsolute};
		Hash style;
	};

	Header header;
	Content content;
	Footer footer;
	ButtonInfo buttonInfo;
	Hash fontURI = defaultFontURI;
};

Dialogue::Dialogue(not_null<ViewStack*> parent, std::string name, CreateInfo const& info)
	: View(parent, std::move(name), Block::eBlock), m_buttonInfo(info.buttonInfo), m_fontURI(info.fontURI) {
	m_content = &push<Button>(m_fontURI, info.content.style);
	m_content->m_rect.size = info.content.size;
	m_content->m_text->set(info.content.text).height(info.content.textHeight);
	m_content->m_interact = false;

	m_header.title = &m_content->push<Button>(m_fontURI, info.header.style);
	m_header.title->m_rect.size = {info.content.size.x, info.header.height};
	m_header.title->m_rect.anchor.norm.y = 0.5f;
	m_header.title->m_rect.anchor.offset.y = info.header.height * 0.5f;
	m_header.title->m_style.widget.quad.base.Tf = info.header.background;
	m_header.title->m_text->set(info.header.text).height(info.header.textHeight);
	m_header.title->m_style.widget.quad.reset(InteractStatus::eHover);
	// m_header.title->m_interact = false;
	m_header.close = &m_header.title->push<Button>(m_fontURI);
	m_header.close->m_style.widget.quad.base.Tf = colours::red;
	m_header.close->m_style.base.text.colour = colours::white;
	m_header.close->m_text->set("x").height(20U);
	m_header.close->m_rect.size = {20.0f, 20.0f};
	m_header.close->m_rect.anchor.norm.x = 0.5f;
	m_header.close->m_rect.anchor.offset.x = -20.0f;
	m_closeSignal = m_header.close->onClick();
	m_closeSignal += [this]() { setDestroyed(); };

	m_footer.bg = &m_content->push<Widget>(info.footer.style);
	m_footer.bg->m_rect.size = {info.content.size.x, info.footer.height};
	m_footer.bg->m_style.widget.quad.base.Tf = info.footer.background;
	m_footer.bg->m_rect.anchor.norm.y = -0.5f;
	m_footer.bg->m_rect.anchor.offset.y = info.footer.height * -0.5f;
	m_footer.bg->m_interact = false;
}

Widget::OnClick::handle Dialogue::addButton(std::string text, Widget::OnClick::callback&& onClick) {
	auto& button = m_footer.bg->push<Button>(m_fontURI, m_buttonInfo.style);
	m_footer.buttons.push_back(&button);
	button.m_rect.anchor.norm.x = -0.5f;
	button.m_rect.size = m_buttonInfo.size;
	button.m_cornerRadius = 10.0f;
	button.m_text->set(std::move(text)).height(m_buttonInfo.textHeight);
	f32 const pad = (m_content->m_rect.size.x - f32(m_footer.buttons.size()) * m_buttonInfo.size.x) / f32(m_footer.buttons.size() + 1);
	f32 offset = pad + m_buttonInfo.size.x * 0.5f;
	for (auto btn : m_footer.buttons) {
		btn->m_rect.anchor.offset.x = offset;
		offset += (pad + m_buttonInfo.size.x);
	}
	auto ret = button.onClick();
	ret += std::move(onClick);
	return ret;
}

void Dialogue::onUpdate(input::Frame const& frame) {
	auto const st = m_header.title->m_previous.status;
	bool const move = st == InteractStatus::eHold;
	if (move && !m_originDelta.moving) {
		m_originDelta.start = frame.state.cursor.position;
		m_originDelta.prev = m_originDelta.delta;
	}
	m_originDelta.moving = move;
	if (m_originDelta.moving) { m_originDelta.delta = m_originDelta.prev + frame.state.cursor.position - m_originDelta.start; }
	m_rect.origin += m_originDelta.delta;
}
} // namespace le::gui

namespace le::demo {
struct EmitMesh {
	MeshPrimitive primitive;
	Material material;
	QuadEmitter emitter;

	EmitMesh(not_null<graphics::VRAM*> vram, EmitterInfo info = {}) : primitive(vram, graphics::MeshPrimitive::Type::eDynamic) { emitter.create(info); }

	void tick(Time_s dt, Opt<dts::task_queue> tasks = {}) {
		emitter.tick(dt, tasks);
		primitive.construct(emitter.geometry());
	}

	MeshView mesh() const { return MeshObj{&primitive, &material}; }
};

class App : public input::Receiver, public SceneRegistry {
  public:
	using Tweener = utils::Tweener<f32, utils::TweenEase>;

	App(not_null<Engine*> eng)
		: m_eng(eng), m_renderer(&eng->gfx().boot.vram),
		  m_testTex(&eng->gfx().boot.vram, eng->store().find<graphics::Sampler>("samplers/no_mip_maps")->sampler(), colours::red, {128, 128}),
		  m_emitter(&eng->gfx().boot.vram) {
		// auto const io = m_tasks.add_queue();
		// m_tasks.add_agent({io, 0});
		// m_manifest.m_jsonQID = io;
		// m_manifest.flags().set(AssetManifest::Flag::eImmediate);
		m_manifest.flags().set(AssetManifest::Flag::eOverwrite);
		auto const res = m_manifest.load("demo", &m_tasks);
		ENSURE(res > 0, "Manifest missing/empty");

		/* custom meshes */ {
			auto rQuad = m_eng->store().add<graphics::MeshPrimitive>("meshes/rounded_quad", graphics::MeshPrimitive(&m_eng->gfx().boot.vram));
			rQuad->construct(graphics::makeRoundedQuad());
		}

		m_eng->pushReceiver(this);

		auto ifreecam = [](edi::Inspect<FreeCam> inspect) { edi::TWidget<f32>("Speed", inspect.get().m_params.xz_speed); };
		auto ipc = [](edi::Inspect<PlayerController> inspect) { edi::TWidget<bool>("Active", inspect.get().active); };
		edi::Inspector::attach<FreeCam>(ifreecam);
		edi::Inspector::attach<SpringArm>(SpringArm::inspect);
		edi::Inspector::attach<PlayerController>(ipc);
		edi::Inspector::attach<gui::Dialogue>([](edi::Inspect<gui::Dialogue>) { edi::Text("Dialogue found!"); });

		{ spawnMesh<Skybox>("skybox", "skyboxes/sky_dusk", "render_pipelines/skybox"); }
		{
			m_data.player = spawnMesh("player", MeshProvider::make("meshes/cube", "materials/player/cube"), "render_pipelines/lit");
			m_registry.get<Transform>(m_data.player).position({0.0f, 0.0f, 5.0f});
			m_registry.attach<PlayerController>(m_data.player);
			auto& trigger = m_registry.attach<physics::Trigger>(m_data.player);
			m_onCollide = trigger.onTrigger.make_signal();
			m_onCollide += [](auto&&) { logD("Collided!"); };
		}
		{
			auto ent0 = spawnMesh<Model>("model_0_0", "models/plant", "render_pipelines/lit");
			m_registry.get<Transform>(ent0).position({-2.0f, -1.0f, 2.0f});
			m_data.entities["model_0_0"] = ent0;

			auto ent1 = spawnMesh<Model>("model_0_1", "models/plant", "render_pipelines/lit");
			auto& node = m_registry.get<Transform>(ent1);
			node.position({-2.0f, -1.0f, 5.0f});
			m_data.entities["model_0_1"] = ent1;
			m_registry.get<SceneNode>(ent1).parent(m_registry, m_data.entities["model_0_0"]);
		}
		{
			auto ent0 = spawnMesh<Model>("model_1_0", "models/teapot", "render_pipelines/lit");
			m_registry.get<Transform>(ent0).position({2.0f, -1.0f, 2.0f});
			m_data.entities["model_1_0"] = ent0;
			auto ent = spawnMesh<Model>("model_1", "models/nanosuit", "render_pipelines/lit");
			m_registry.get<Transform>(ent).position({-1.0f, -2.0f, -3.0f});
			m_data.entities["model_1"] = ent;
		}
		{
			m_data.camera = m_registry.make_entity<FreeCam>("freecam");
			auto [e, c] = SpringArm::attach(m_data.camera, m_registry, m_data.player);
			auto& [spring, transform] = c;
			edi::SceneTree::attach(m_data.camera);
			auto& cam = m_registry.get<FreeCam>(m_data.camera);
			cam.position = {0.0f, 0.5f, 4.0f};
			cam.look({});
			transform.position(cam.position);
			spring.offset = transform.position();
			m_registry.get<graphics::Camera>(m_sceneRoot) = cam;
		}
		{
			auto ent = spawnMesh("prop_1", MeshProvider::make("meshes/cube"), "render_pipelines/basic");
			m_registry.get<Transform>(ent).position({-5.0f, -1.0f, -2.0f});
			m_data.entities["prop_1"] = ent;
		}
		{
			auto ent = spawnMesh("prop_2", MeshProvider::make("meshes/cone"), "render_pipelines/tex");
			m_registry.get<Transform>(ent).position({1.0f, -2.0f, -3.0f});
		}
		{
			m_testMat.map_Kd = &m_testTex;
			if (auto primitive = m_eng->store().find<MeshPrimitive>("meshes/rounded_quad")) {
				m_data.roundedQuad = spawnNode("prop_3");
				m_registry.attach<RenderPipeProvider>(m_data.roundedQuad, RenderPipeProvider::make("render_pipelines/tex"));
				m_registry.attach<MeshView>(m_data.roundedQuad, MeshObj{&*primitive, &m_testMat});
				m_registry.get<Transform>(m_data.roundedQuad).position({2.0f, 0.0f, 6.0f});
			}
		}
		{
			Material mat;
			mat.Tf = colours::yellow;
			m_eng->store().add("materials/yellow", mat);
			auto node = spawnMesh("trigger/cube", MeshProvider::make("meshes/cube", "materials/yellow"), "render_pipelines/basic");
			m_registry.get<Transform>(node).scale(2.0f);
			m_data.tween = node;
			auto& trig1 = m_registry.attach<physics::Trigger>(node);
			trig1.scale = glm::vec3(2.0f);
			auto& tweener = m_registry.attach<Tweener>(node, -5.0f, 5.0f, 2s, utils::TweenCycle::eSwing);
			auto pos = m_registry.get<Transform>(node).position();
			pos.x = tweener.current();
			m_registry.get<Transform>(node).position(pos);
		}
		{
			auto ent = spawnNode("emitter");
			m_registry.attach<DynamicMesh>(ent, DynamicMesh::make(&m_emitter));
			m_registry.attach<RenderPipeProvider>(ent, RenderPipeProvider::make("render_pipelines/ui"));
		}
		{
			DirLight l0, l1;
			l0.direction = {-graphics::up, 0.0f};
			l1.direction = {-graphics::front, 0.0f};
			l0.albedo = Albedo::make(colours::cyan, {0.2f, 0.5f, 0.3f, 0.0f});
			l1.albedo = Albedo::make(colours::white, {0.4f, 1.0f, 0.8f, 0.0f});
			m_data.dirLights = {l0, l1};
		}
		m_data.guiStack = spawn<gui::ViewStack>("gui_root", "render_pipelines/ui", &m_eng->gfx().boot.vram);
	}

	bool block(input::State const& state) override {
		if (m_controls.editor(state)) { m_eng->editor().toggle(); }
		if (m_controls.wireframe(state)) {
			/*if (auto lit = m_eng->store().find<PipelineState>("pipelines/lit")) {
				if (lit->fixedState.flags.test(graphics::PFlag::eWireframe)) {
					lit->fixedState.flags.reset(graphics::PFlag::eWireframe);
					lit->fixedState.lineWidth = 1.0f;
				} else {
					lit->fixedState.flags.set(graphics::PFlag::eWireframe);
					lit->fixedState.lineWidth = 3.0f;
				}
			}*/
		}
		if (m_controls.reboot(state)) { m_data.reboot = true; }
		if (m_controls.unload(state)) {
			m_data.unloaded = true;
			m_registry.clear();
			logI("{} unloaded", m_manifest.unload("demo", m_tasks));
		}
		return false;
	}

	void init1() {
		if constexpr (levk_debug) {
			auto triggerDebug = m_registry.make_entity<physics::Trigger::Debug>("trigger_debug");
			m_registry.attach<RenderPipeProvider>(triggerDebug, RenderPipeProvider::make("render_pipelines/wireframe"));
		}

		if (auto font = m_eng->store().find<graphics::Font>("fonts/vera_serif")) {
			m_data.text.emplace(&*font);
			m_data.text->m_colour = colours::yellow;
			TextMesh::Line line;
			line.layout.scale = font->scale(80U);
			line.layout.origin.y = 200.0f;
			line.layout.pivot = font->pivot(graphics::Font::Align::eCentre, graphics::Font::Align::eCentre);
			line.line = "Hello!";
			m_data.text->m_info = std::move(line);
			auto ent = spawnNode("text");
			m_registry.attach<DynamicMesh>(ent, DynamicMesh::make(&*m_data.text));
			m_registry.attach<RenderPipeProvider>(ent, RenderPipeProvider::make("render_pipelines/ui"));

			m_data.cursor.emplace(&*font);
			m_data.cursor->m_colour = colours::yellow;
			m_data.cursor->m_layout.scale = font->scale(80U);
			m_data.cursor->m_layout.origin.y = 200.0f;
			m_data.cursor->m_layout.pivot = font->pivot(graphics::Font::Align::eCentre, graphics::Font::Align::eCentre);
			m_data.cursor->m_line = "Hello!";
			m_data.text->m_info = m_data.cursor->generateText();
			auto ent1 = spawnNode("text_cursor");
			m_registry.attach<DynamicMesh>(ent1, DynamicMesh::make(&*m_data.cursor));
			m_registry.attach<RenderPipeProvider>(ent1, RenderPipeProvider::make("render_pipelines/ui"));
		}

		auto& stack = m_registry.get<gui::ViewStack>(m_data.guiStack);
		[[maybe_unused]] auto& testView = stack.push<TestView>("test_view");
		gui::Dropdown::CreateInfo dci;
		dci.flexbox.background.Tf = RGBA(0x888888ff, RGBA::Type::eAbsolute);
		// dci.quadStyle.at(gui::InteractStatus::eHover).Tf = colours::cyan;
		dci.textHeight = 30U;
		dci.options = {"zero", "one", "two", "/bthree", "four"};
		dci.selected = 2;
		auto& dropdown = testView.push<gui::Dropdown>(std::move(dci));
		dropdown.m_rect.anchor.offset = {-300.0f, -50.0f};
		gui::Dialogue::CreateInfo gdci;
		gdci.header.text = "Dialogue";
		gdci.content.text = "Content\ngoes\nhere";
		auto& dialogue = stack.push<gui::Dialogue>("test_dialogue", gdci);
		gui::InputField::CreateInfo info;
		// info.secret = true;
		auto& in = dialogue.push<gui::InputField>(info);
		in.m_rect.anchor.offset.y = 60.0f;
		m_data.btnSignals.push_back(dialogue.addButton("OK", [&dialogue]() { dialogue.setDestroyed(); }));
		m_data.btnSignals.push_back(dialogue.addButton("Cancel", [&dialogue]() { dialogue.setDestroyed(); }));

		if (auto model = m_eng->store().find<Model>("models/teapot")) { model->material(0)->Tf = {0xfc4340ff, RGBA::Type::eAbsolute}; }
		m_data.init = true;

		if (auto tex = m_eng->store().find<graphics::Texture>("textures/awesomeface.png")) { m_emitter.material.map_Kd = &*tex; }
	}

	bool reboot() const noexcept { return m_data.reboot; }

	void tick(Time_s dt) {
		// if (m_data.tgMesh && m_data.tgCursor) {
		// 	graphics::Geometry geom;
		// 	if (m_data.tgCursor->update(m_eng->inputFrame().state, &geom)) { m_data.tgMesh->primitive.construct(std::move(geom)); }
		// 	if (!m_data.tgCursor->m_flags.test(input::TextCursor::Flag::eActive) && m_eng->inputFrame().state.pressed(input::Key::eEnter)) {
		// 		m_data.tgCursor->m_flags.set(input::TextCursor::Flag::eActive);
		// 	}
		// }

		if (m_data.text && m_data.cursor) {
			m_data.cursor->update(m_eng->inputFrame().state, &m_data.text->m_info.get<graphics::Geometry>());
			if (!m_data.cursor->m_flags.test(input::TextCursor2::Flag::eActive) && m_eng->inputFrame().state.pressed(input::Key::eEnter)) {
				m_data.cursor->m_flags.set(input::TextCursor2::Flag::eActive);
			}
		}

		if (auto const frame = m_eng->gfx().context.renderer().offScreenImage(); frame && m_eng->gfx().context.renderer().canBlitFrame()) {
			auto cmd = graphics::InstantCommand(&m_eng->gfx().boot.vram);
			m_testTex.blit(cmd.cb(), frame->ref());
		}

		updateSystems(m_tasks, dt, &m_eng->inputFrame());
		if (!m_data.unloaded) {
			auto pr_ = Engine::profile("app::tick");
			// static constexpr u32 max_cp = 128;
			// if (m_data.init && m_built.value < max_cp && !m_thread.active()) {
			// 	m_thread = ktl::kthread([this] {
			// 		auto inst = graphics::InstantCommand(&m_eng->gfx().boot.vram);
			// 		while (m_built.value < max_cp) { m_atlas.build(inst.cb(), m_built.value++); }
			// 		// m_eng->store().find<Material>("materials/atlas_quad")->map_Kd = &m_atlas.texture();
			// 	});
			// }
			// if (m_data.init && m_built.value < max_cp) {
			// 	auto inst = graphics::InstantCommand(&m_eng->gfx().boot.vram);
			// 	m_atlas.build(inst.cb(), m_built.value++);
			// }
			if (!m_data.init && m_manifest.ready(m_tasks)) { init1(); }
			auto& cam = m_registry.get<FreeCam>(m_data.camera);
			m_registry.get<graphics::Camera>(m_sceneRoot) = cam;
			auto& pc = m_registry.get<PlayerController>(m_data.player);
			auto const& state = m_eng->inputFrame().state;
			if (pc.active) {
				auto& transform = m_registry.get<Transform>(m_data.player);
				pc.tick(state, transform, dt);
				auto const forward = nvec3(transform.orientation() * -graphics::front);
				cam.face(forward);
				cam.position = m_registry.get<Transform>(m_data.camera).position();
			} else {
				cam.tick(state, dt, &m_eng->window());
			}
			m_registry.get<Transform>(m_data.entities["prop_1"]).rotate(glm::radians(360.0f) * dt.count(), graphics::up);
			if (auto tr = m_registry.find<Transform>(m_data.entities["model_0_0"])) { tr->rotate(glm::radians(-75.0f) * dt.count(), graphics::up); }
			/*if (auto tr = m_registry.find<Transform>(m_data.entities["model_1_0"])) {
				static glm::quat s_axis = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
				s_axis = glm::rotate(s_axis, glm::radians(45.0f) * dt.count(), graphics::front);
				tr->rotate(glm::radians(90.0f) * dt.count(), nvec3(s_axis * graphics::up));
			}*/
			if (auto tr = m_registry.find<Transform>(m_data.tween)) {
				auto& tweener = m_registry.get<Tweener>(m_data.tween);
				auto pos = tr->position();
				pos.x = tweener.tick(dt);
				tr->position(pos);
			}

			m_emitter.tick(dt, &m_tasks);
		}

		m_tasks.rethrow();
	}

	void render() {
		// draw
		if (auto rp = m_eng->beginRenderPass(this, RGBA(0x777777ff, RGBA::Type::eAbsolute))) {
			Renderer::Scene scene;
			scene.registry = &m_registry;
			scene.camera = m_registry.find<graphics::Camera>(m_sceneRoot);
			scene.lights = m_data.dirLights;
			scene.size = m_eng->sceneSpace();
			m_renderer.render(*rp, scene);
			m_eng->endRenderPass(*rp);
		}
	}

	scheduler& sched() { return m_tasks; }

  private:
	struct Data {
		std::unordered_map<Hash, dens::entity> entities;

		std::optional<TextMesh> text;
		std::optional<input::TextCursor2> cursor;
		std::vector<DirLight> dirLights;
		std::vector<gui::Widget::OnClick::handle> btnSignals;

		dens::entity camera;
		dens::entity player;
		dens::entity guiStack;
		dens::entity tween;
		dens::entity roundedQuad;
		bool reboot = false;
		bool unloaded = {};
		bool init{};
	};

	Data m_data;
	scheduler m_tasks;
	AssetManifest m_manifest;
	not_null<Engine*> m_eng;
	mutable Renderer m_renderer;
	Material m_testMat;
	graphics::Texture m_testTex;
	physics::OnTrigger::handle m_onCollide;
	EmitMesh m_emitter;

	struct {
		input::Trigger editor = {input::Key::eE, input::Action::ePress, input::Mod::eCtrl};
		input::Trigger wireframe = {input::Key::eP, input::Action::ePress, input::Mod::eCtrl};
		input::Trigger reboot = {input::Key::eR, input::Action::ePress, input::Mods(input::Mod::eCtrl, input::Mod::eShift)};
		input::Trigger unload = {input::Key::eU, input::Action::ePress, input::Mods(input::Mod::eCtrl, input::Mod::eShift)};
	} m_controls;
};

struct FlagsInput : input::Receiver {
	Flags& flags;

	FlagsInput(Flags& flags) : flags(flags) {}

	bool block(input::State const& state) override {
		bool ret = false;
		if (auto w = state.released(input::Key::eW); w && w->test(input::Mod::eCtrl)) {
			flags.set(Flag::eClosed);
			ret = true;
		}
		if (auto d = state.released(input::Key::eD); d && d->test(input::Mod::eCtrl)) {
			flags.set(Flag::eDebug0);
			ret = true;
		}
		return ret;
	}
};

bool openFilesystemPath(io::Path path) {
	if (io::is_regular_file(path)) {
		path = path.parent_path();
	} else if (!io::is_directory(path)) {
		return false;
	}
	std::string const dir = path.string();
	if constexpr (levk_OS == os::OS::eLinux) {
		static constexpr std::string_view mgrs[] = {"dolphin", "nautilus"};
		for (auto const mgr : mgrs) {
			if (auto open = utils::Shell(fmt::format("{} {}", mgr, dir).data())) { return true; }
		}
		return false;
	} else if constexpr (levk_OS == os::OS::eWindows) {
		return utils::Shell(fmt::format("explorer {}", dir).data()).success();
	}
}

bool package(io::Path const& binary, bool clean) {
	logD("Starting build...");
	io::Path const log = binary / "autobuild.txt";
	io::remove(log);
	auto const logFile = log.string();
	auto const binPath = io::absolute(binary).string();
	if (!io::is_regular_file(binary / "CMakeCache.txt")) {
		if (!utils::Shell(fmt::format("cmake-gui -B {}", binPath).data(), logFile.data())) { return false; }
		if (!io::is_regular_file(binary / "CMakeCache.txt")) { return false; }
	}
	auto const cmake = utils::Shell(fmt::format("cmake --build {} {}", binPath, clean ? "--clean-first" : "").data(), logFile.data());
	if (!cmake) {
		logW("Build failure: \n{}", cmake.output());
		return false;
	}
	logD("... Build completed");
	return openFilesystemPath(binary);
}

bool run(io::Media const& media) {
	Engine::CreateInfo eci;
	eci.winInfo.config.title = "levk demo";
	eci.winInfo.config.size = {1280, 720};
	eci.winInfo.options.centreCursor = true;
	Engine engine(eci, &media);
	Flags flags;
	FlagsInput flagsInput(flags);
	engine.pushReceiver(&flagsInput);
	bool reboot = false;
	Engine::Boot::CreateInfo bootInfo;
	if constexpr (levk_debug) { bootInfo.device.instance.validation = graphics::Validation::eOn; }
	bootInfo.device.validationLogLevel = LogLevel::info;
	do {
		engine.boot(bootInfo);
		App app(&engine);
		DeltaTime dt;
		ktl::kfuture<bool> bf;
		ktl::kasync async;
		while (!engine.closing()) {
			if (!engine.nextFrame()) { continue; }
			poll(flags, engine.poll(true).residue);
			if (flags.test(Flag::eClosed)) {
				reboot = false;
				break;
			}
			if (app.reboot()) {
				reboot = true;
				break;
			}
			app.tick(++dt);
			app.render();
			if (flags.test(Flag::eDebug0) && (!bf.valid() || !bf.busy())) {
				// app.sched().enqueue([]() { ENSURE(false, "test"); });
				// app.sched().enqueue([]() { ENSURE(false, "test2"); });
				auto& ctx = engine.gfx().context;
				if (auto img = graphics::utils::makeStorage(&ctx.vram(), ctx.previousFrame().ref())) {
					if (auto file = std::ofstream("shot.ppm", std::ios::out | std::ios::binary)) {
						auto const written = graphics::utils::writePPM(ctx.vram().m_device, *img, file);
						if (written > 0) { logD("Screenshot saved to shot.ppm"); }
					}
				}
			}
			flags.reset(Flag::eDebug0);
			/*bf = async(&package, "out/autobuild", false);
			bf.then([](bool built) {
				if (!built) {
					logW("build failed");
				} else {
					logD("build success");
				}
			});*/
		}
	} while (reboot);
	return true;
}
} // namespace le::demo
