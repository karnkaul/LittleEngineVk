#include <levk/core/not_null.hpp>
#include <levk/core/utils/algo.hpp>
#include <levk/core/utils/std_hash.hpp>
#include <levk/core/utils/string.hpp>
#include <levk/engine/input/control.hpp>
#include <levk/gameplay/cameras/freecam.hpp>
#include <levk/graphics/common.hpp>
#include <levk/graphics/render/shader_buffer.hpp>
#include <levk/graphics/utils/utils.hpp>

#include <levk/engine/utils/exec.hpp>
#include <levk/gameplay/gui/quad.hpp>
#include <levk/gameplay/gui/text.hpp>
#include <levk/gameplay/gui/view.hpp>
#include <levk/gameplay/gui/widget.hpp>

#include <levk/gameplay/gui/widgets/dropdown.hpp>

#include <ktl/async/kasync.hpp>
#include <levk/core/utils/shell.hpp>
#include <levk/core/utils/tween.hpp>
#include <levk/engine/input/text_cursor.hpp>
#include <levk/engine/render/quad_emitter.hpp>
#include <levk/engine/render/text_mesh.hpp>
#include <levk/gameplay/gui/widgets/input_field.hpp>
#include <levk/graphics/font/font.hpp>
#include <levk/graphics/utils/instant_command.hpp>
#include <fstream>

#include <dumb_tasks/executor.hpp>
#include <ktl/async/kthread.hpp>
#include <levk/engine/builder.hpp>
#include <levk/engine/render/shader_data.hpp>
#include <levk/gameplay/ecs/components/spring_arm.hpp>
#include <levk/gameplay/ecs/components/trigger.hpp>
#include <levk/graphics/render/context.hpp>

#include <levk/engine/assets/asset_manifest.hpp>
#include <levk/gameplay/editor/editor.hpp>
#include <levk/gameplay/editor/inspector.hpp>
#include <levk/gameplay/editor/scene_tree.hpp>
#include <levk/gameplay/scene/list_renderer.hpp>
#include <levk/gameplay/scene/scene_manager.hpp>

#include <levk/graphics/mesh.hpp>
#include <levk/graphics/skybox.hpp>

namespace le::demo {
using RGBA = graphics::RGBA;

enum class Flag { eQuit, eReboot, eClose, eDebug0 };
using Flags = ktl::enum_flags<Flag, u8>;

using namespace dts;

using namespace std::chrono;

struct Albedo {
	alignas(16) glm::vec4 ambient;
	alignas(16) glm::vec4 diffuse;
	alignas(16) glm::vec4 specular;

	static Albedo make(RGBA colour = colours::white, glm::vec4 const& amdispsh = {0.5f, 0.8f, 0.4f, 42.0f}) noexcept { return make(colour.toVec4(), amdispsh); }

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

	struct SceneData {
		RenderMap custom;
		ShaderSceneView view;
		Span<DirLight const> lights;
	};

	void render(RenderPass& out_rp, ShaderBufferMap& sbMap, SceneData data, AssetStore const& store, dens::registry const& registry) {
		m_mats = &sbMap.get("mats");
		m_mats->write(data.view);
		m_lights = &sbMap.get("lights");
		if (!data.lights.empty()) {
			DirLights dl;
			for (std::size_t idx = 0; idx < data.lights.size() && idx < dl.lights.size(); ++idx) { dl.lights[idx] = data.lights[idx]; }
			dl.count = std::min((u32)data.lights.size(), (u32)dl.lights.size());
			m_lights->write(dl);
		} else {
			m_lights->write(DirLights());
		}
		auto& map = data.custom;
		fill(map, store, registry);
		ListRenderer::render(out_rp, store, std::move(map));
	}

  private:
	void writeSets(graphics::DescriptorMap map, graphics::DrawList const& list) override {
		auto set0 = map.nextSet(list.m_bindings, 0);
		set0.update(0, *m_mats);
		set0.update(1, *m_lights);
		for (auto const& drawObj : list) {
			map.nextSet(drawObj.bindings, 1).update(0, drawObj.matrix);
			for (auto const& obj : drawObj.objs) {
				auto const& primitive = obj.primitive;
				auto const smat = primitive.blinnPhong ? primitive.blinnPhong->std140() : graphics::BPMaterialData::Std140{};
				auto set2 = map.nextSet(obj.bindings, 2);
				set2.update(0, primitive.textures[MatTexType::eDiffuse]);
				set2.update(1, primitive.textures[MatTexType::eAlpha]);
				set2.update(2, primitive.textures[MatTexType::eSpecular]);
				map.nextSet(obj.bindings, 3).update(0, smat);
			}
		}
	}

	graphics::ShaderBuffer* m_mats{};
	graphics::ShaderBuffer* m_lights{};
};

class TestView : public gui::View {
  public:
	TestView(not_null<gui::ViewStack*> parent, std::string name, Hash fontURI = gui::defaultFontURI) : gui::View(parent, std::move(name)) {
		m_canvas.size.value = {1280.0f, 720.0f};
		m_canvas.size.unit = gui::Unit::eAbsolute;
		auto& bg = push<gui::Quad>();
		bg.m_rect.size = {200.0f, 100.0f};
		bg.m_rect.anchor.norm = {-0.25f, 0.25f};
		bg.m_bpMaterial.Tf = colours::cyan;
		auto& centre = bg.push<gui::Quad>();
		centre.m_rect.size = {50.0f, 50.0f};
		centre.m_bpMaterial.Tf = colours::red;
		auto& dot = centre.push<gui::Quad>();
		dot.offset({30.0f, 20.0f});
		dot.m_bpMaterial.Tf = Colour(0x333333ff);
		dot.m_rect.anchor.norm = {-0.5f, -0.5f};
		auto& topLeft = bg.push<gui::Quad>();
		topLeft.m_rect.anchor.norm = {-0.5f, 0.5f};
		topLeft.offset({25.0f, 25.0f}, {1.0f, -1.0f});
		topLeft.m_bpMaterial.Tf = colours::magenta;
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
	graphics::MeshPrimitive primitive;
	TextureRefs textures;
	graphics::BPMaterialData material;
	QuadEmitter emitter;

	EmitMesh(not_null<graphics::VRAM*> vram, EmitterInfo info = {}) : primitive(vram, graphics::MeshPrimitive::Type::eDynamic) { emitter.create(info); }

	void tick(Time_s dt, Opt<dts::executor> executor = {}) {
		emitter.tick(dt, executor);
		primitive.construct(emitter.geometry());
	}

	void addDrawPrimitives(AssetStore const& store, graphics::DrawList& out, glm::mat4 const& matrix) const {
		graphics::MaterialTextures matTex;
		textures.fill(store, matTex);
		out.push(graphics::DrawPrimitive{matTex, &primitive, &material}, matrix);
	}
};

class App : public input::Receiver, public Scene {
  public:
	using Tweener = utils::Tweener<f32, utils::TweenEase>;

	App(Engine::Service const& eng)
		: m_manifest(eng), m_testTex(&eng.vram(), eng.store().find<graphics::Sampler>("samplers/no_mip_maps")->sampler(), colours::red, {128, 128}),
		  m_emitter(&eng.vram()) {}

	void open() override {
		Scene::open();

		m_manifest.load("demo.manifest");
		ENSURE(!m_manifest.manifest().list.empty(), "Manifest missing/empty");

		/* custom meshes */ {
			auto rQuad = engine().store().add<graphics::MeshPrimitive>("mesh_primitives/rounded_quad", graphics::MeshPrimitive(&engine().vram()));
			rQuad->construct(graphics::makeRoundedQuad());
		}

		engine().pushReceiver(this);

		auto ifreecam = [](editor::Inspect<FreeCam> inspect) { editor::TWidget<f32>("Speed", inspect.get().m_params.xz_speed); };
		auto ipc = [](editor::Inspect<PlayerController> inspect) { editor::TWidget<bool>("Active", inspect.get().active); };
		editor::Inspector::attach<FreeCam>(ifreecam);
		editor::Inspector::attach<SpringArm>(SpringArm::inspect);
		editor::Inspector::attach<PlayerController>(ipc);
		editor::Inspector::attach<gui::Dialogue>([](editor::Inspect<gui::Dialogue>) { editor::Text("Dialogue found!"); });

		{
			auto skybox = spawnNode("skybox");
			m_registry.attach(skybox, AssetProvider<graphics::Skybox>("skyboxes/sky_dusk"));
			m_registry.attach(skybox, RenderPipeProvider("render_pipelines/skybox"));
		}
		{
			PrimitiveProvider provider("mesh_primitives/cube", "materials/bp/player/cube", "texture_uris/player/cube");
			m_data.player = spawn("player", provider, "render_pipelines/lit");
			m_registry.attach(m_data.player, std::move(provider));
			m_registry.get<Transform>(m_data.player).position({0.0f, 0.0f, 5.0f});
			m_registry.attach<PlayerController>(m_data.player);
			auto& trigger = m_registry.attach<physics::Trigger>(m_data.player);
			m_onCollide = trigger.onTrigger.make_signal();
			m_onCollide += [](auto&&) { logD("Collided!"); };
		}
		{
			auto ent0 = spawn("model_0_0", AssetProvider<graphics::Mesh>("meshes/plant"), "render_pipelines/lit");
			m_registry.get<Transform>(ent0).position({-2.0f, -1.0f, 2.0f});
			m_data.entities["model_0_0"] = ent0;

			auto ent1 = spawn("model_0_1", AssetProvider<graphics::Mesh>("meshes/plant"), "render_pipelines/lit");
			auto& node = m_registry.get<Transform>(ent1);
			node.position({-2.0f, -1.0f, 5.0f});
			m_data.entities["model_0_1"] = ent1;
			m_registry.get<SceneNode>(ent1).parent(m_registry, m_data.entities["model_0_0"]);
		}
		{
			auto ent0 = spawn("model_1_0", AssetProvider<graphics::Mesh>("meshes/teapot"), "render_pipelines/lit");
			m_registry.get<Transform>(ent0).position({2.0f, -1.0f, 2.0f});
			m_data.entities["model_1_0"] = ent0;
			auto ent = spawn("model_1", AssetProvider<graphics::Mesh>("meshes/nanosuit"), "render_pipelines/lit");
			m_registry.get<Transform>(ent).position({-1.0f, -2.0f, -3.0f});
			m_data.entities["model_1"] = ent;
		}
		{
			m_data.camera = m_registry.make_entity<FreeCam>("freecam");
			auto [e, c] = SpringArm::attach(m_data.camera, m_registry, m_data.player);
			auto& [spring, transform] = c;
			editor::SceneTree::attach(m_data.camera);
			auto& cam = m_registry.get<FreeCam>(m_data.camera);
			cam.position = {0.0f, 0.5f, 4.0f};
			cam.look({});
			transform.position(cam.position);
			spring.offset = transform.position();
			m_registry.get<graphics::Camera>(m_sceneRoot) = cam;
		}
		{
			auto ent = spawn("prop_1", PrimitiveProvider("mesh_primitives/cube"), "render_pipelines/basic");
			m_registry.get<Transform>(ent).position({-5.0f, -1.0f, -2.0f});
			m_data.entities["prop_1"] = ent;
		}
		{
			auto ent = spawn("prop_2", PrimitiveProvider("mesh_primitives/cone"), "render_pipelines/tex");
			m_registry.get<Transform>(ent).position({1.0f, -2.0f, -3.0f});
		}
		{
			if (auto primitive = engine().store().find<graphics::MeshPrimitive>("mesh_primitives/rounded_quad")) {
				m_data.roundedQuad = spawnNode("prop_3");
				m_registry.attach(m_data.roundedQuad, RenderPipeProvider("render_pipelines/tex"));
				m_registry.get<Transform>(m_data.roundedQuad).position({2.0f, 0.0f, 6.0f});
				auto const mat = engine().store().find<graphics::BPMaterialData>("materials/bp/default");
				auto addDrawPrims = [this, primitive, mat](graphics::DrawList& out, glm::mat4 const& matrix) {
					graphics::MaterialTextures matTex;
					matTex[graphics::MatTexType::eDiffuse] = &m_testTex;
					out.push(graphics::DrawPrimitive{matTex, primitive, mat}, matrix);
				};
				m_registry.attach(m_data.roundedQuad, PrimitiveGenerator(addDrawPrims));
			}
		}
		{
			graphics::BPMaterialData mat;
			mat.Tf = colours::yellow;
			engine().store().add("materials/bp/yellow", mat);
			auto node = spawn("trigger/cube", PrimitiveProvider("mesh_primitives/cube", "materials/bp/yellow"), "render_pipelines/basic");
			m_registry.get<Transform>(node).scale(2.0f);
			m_data.tween = node;
			auto& trig1 = m_registry.attach<physics::Trigger>(node);
			trig1.scale = glm::vec3(2.0f);
			auto& tweener = m_registry.attach(node, Tweener(-5.0f, 5.0f, 2s, utils::TweenCycle::eSwing));
			auto pos = m_registry.get<Transform>(node).position();
			pos.x = tweener.current();
			m_registry.get<Transform>(node).position(pos);
		}
		{
			auto ent = spawnNode("emitter");
			m_emitter.textures.textures[graphics::MatTexType::eDiffuse] = "textures/awesomeface.png";
			auto addPrims = [this](graphics::DrawList& out, glm::mat4 const& matrix) { m_emitter.addDrawPrimitives(engine().store(), out, matrix); };
			m_registry.attach(ent, PrimitiveGenerator(addPrims));
			m_registry.attach(ent, RenderPipeProvider("render_pipelines/ui"));
		}
		{
			DirLight l0, l1;
			l0.direction = {-graphics::up, 0.0f};
			l1.direction = {-graphics::front, 0.0f};
			l0.albedo = Albedo::make(colours::cyan, {0.2f, 0.5f, 0.3f, 0.0f});
			l1.albedo = Albedo::make(colours::white, {0.4f, 1.0f, 0.8f, 0.0f});
			m_data.dirLights = {l0, l1};
		}
		m_data.guiStack = spawn<gui::ViewStack>("gui_root", "render_pipelines/ui", gui::ViewStack(&engine().vram()));
		{
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
		}
	}

	void close() override {
		Scene::close();
		m_manifest.unload();
	}

	bool block(input::State const& state) override {
		if (m_controls.editor(state)) { editor::toggle(); }
		if (m_controls.wireframe(state)) {
			/*if (auto lit = engine().store().find<PipelineState>("pipelines/lit")) {
				if (lit->fixedState.flags.test(graphics::PFlag::eWireframe)) {
					lit->fixedState.flags.reset(graphics::PFlag::eWireframe);
					lit->fixedState.lineWidth = 1.0f;
				} else {
					lit->fixedState.flags.set(graphics::PFlag::eWireframe);
					lit->fixedState.lineWidth = 3.0f;
				}
			}*/
		}
		return false;
	}

	void onAssetsLoaded() {
		if constexpr (levk_debug) {
			auto triggerDebug = m_registry.make_entity<physics::Trigger::Debug>("trigger_debug");
			m_registry.attach(triggerDebug, RenderPipeProvider("render_pipelines/wireframe"));
		}

		if (auto font = engine().store().find<graphics::Font>("fonts/vera_serif")) {
			m_data.text.emplace(&*font);
			m_data.text->m_colour = colours::yellow;
			TextMesh::Line line;
			line.layout.scale = font->scale(80U);
			line.layout.origin.y = 200.0f;
			line.layout.pivot = font->pivot(graphics::Font::Align::eCentre, graphics::Font::Align::eCentre);
			line.line = "Hello!";
			m_data.text->m_info = std::move(line);
			auto ent = spawnNode("text");
			m_registry.attach(ent, PrimitiveGenerator::make(&*m_data.text));
			m_registry.attach(ent, RenderPipeProvider("render_pipelines/ui"));

			m_data.cursor.emplace(font->m_vram);
			m_data.cursor->font(&*font);
			m_data.cursor->m_colour = colours::yellow;
			m_data.cursor->m_layout.scale = font->scale(80U);
			m_data.cursor->m_layout.origin.y = 200.0f;
			m_data.cursor->m_layout.pivot = font->pivot(graphics::Font::Align::eCentre, graphics::Font::Align::eCentre);
			m_data.cursor->m_line = "Hello!";
			m_data.text->m_info = m_data.cursor->generateText();
			auto ent1 = spawnNode("text_cursor");
			m_registry.attach(ent1, PrimitiveGenerator::make(&*m_data.cursor));
			m_registry.attach(ent1, RenderPipeProvider("render_pipelines/ui"));
		}

		if (auto teapot = engine().store().find<graphics::Mesh>("meshes/teapot")) {
			teapot->materials[0].data.get<graphics::BPMaterialData>().Tf = {0xfc4340ff, RGBA::Type::eAbsolute};
		}
		m_data.init = true;
	}

	void tick(Time_s dt) override {
		// if (m_data.tgMesh && m_data.tgCursor) {
		// 	graphics::Geometry geom;
		// 	if (m_data.tgCursor->update(engine().inputFrame().state, &geom)) { m_data.tgMesh->primitive.construct(std::move(geom)); }
		// 	if (!m_data.tgCursor->m_flags.test(input::TextCursor::Flag::eActive) && engine().inputFrame().state.pressed(input::Key::eEnter)) {
		// 		m_data.tgCursor->m_flags.set(input::TextCursor::Flag::eActive);
		// 	}
		// }

		Scene::tick(dt);

		if (m_data.text && m_data.cursor) {
			m_data.cursor->update(engine().inputFrame().state, &m_data.text->m_info.get<graphics::Geometry>());
			if (!m_data.cursor->m_flags.test(input::TextCursor::Flag::eActive) && engine().inputFrame().state.pressed(input::Key::eEnter)) {
				m_data.cursor->m_flags.set(input::TextCursor::Flag::eActive);
			}
		}

		if (auto const frame = engine().context().renderer().offScreenImage(); frame && engine().context().renderer().canBlitFrame()) {
			auto cmd = graphics::InstantCommand(&engine().vram());
			m_testTex.blit(cmd.cb(), frame->ref());
		}

		// static constexpr u32 max_cp = 128;
		// if (m_data.init && m_built.value < max_cp && !m_thread.active()) {
		// 	m_thread = ktl::kthread([this] {
		// 		auto inst = graphics::InstantCommand(&engine().vram());
		// 		while (m_built.value < max_cp) { m_atlas.build(inst.cb(), m_built.value++); }
		// 		// engine().store().find<Material>("materials/atlas_quad")->map_Kd = &m_atlas.texture();
		// 	});
		// }
		// if (m_data.init && m_built.value < max_cp) {
		// 	auto inst = graphics::InstantCommand(&engine().vram());
		// 	m_atlas.build(inst.cb(), m_built.value++);
		// }
		if (!m_data.init && !m_manifest.busy()) { onAssetsLoaded(); }
		auto& cam = m_registry.get<FreeCam>(m_data.camera);
		m_registry.get<graphics::Camera>(m_sceneRoot) = cam;
		auto& pc = m_registry.get<PlayerController>(m_data.player);
		auto const& state = engine().inputFrame().state;
		if (pc.active) {
			auto& transform = m_registry.get<Transform>(m_data.player);
			pc.tick(state, transform, dt);
			auto const forward = nvec3(transform.orientation() * -graphics::front);
			cam.face(forward);
			cam.position = m_registry.get<Transform>(m_data.camera).position();
		} else {
			cam.tick(state, dt, &engine().window());
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

		m_emitter.tick(dt, &executor());
	}

	void render(graphics::RenderPass& renderPass, ShaderSceneView const& view) override {
		// draw
		Renderer::SceneData scene;
		scene.view = view;
		scene.lights = m_data.dirLights;
		Renderer{}.render(renderPass, shaderBufferMap(), scene, engine().store(), m_registry);
	}

  private:
	struct Data {
		std::unordered_map<Hash, dens::entity> entities;

		std::optional<TextMesh> text;
		std::optional<input::TextCursor> cursor;
		std::vector<DirLight> dirLights;
		std::vector<gui::Widget::OnClick::handle> btnSignals;

		dens::entity camera;
		dens::entity player;
		dens::entity guiStack;
		dens::entity tween;
		dens::entity roundedQuad;
		bool init{};
	};

	Data m_data;
	ManifestLoader m_manifest;
	graphics::Texture m_testTex;
	physics::OnTrigger::handle m_onCollide;
	EmitMesh m_emitter;

	struct {
		input::Trigger editor = {input::Key::eE, input::Action::ePress, input::Mod::eCtrl};
		input::Trigger wireframe = {input::Key::eP, input::Action::ePress, input::Mod::eCtrl};
	} m_controls;
};

struct FlagsInput : input::Receiver {
	Flags& flags;
	input::Trigger reboot = {input::Key::eR, input::Action::ePress, input::Mods(input::Mod::eCtrl, input::Mod::eShift)};
	input::Trigger close = {input::Key::eU, input::Action::ePress, input::Mods(input::Mod::eCtrl, input::Mod::eShift)};

	FlagsInput(Flags& flags) : flags(flags) {}

	bool block(input::State const& state) override {
		bool ret = false;
		if (auto w = state.released(input::Key::eW); w && w->test(input::Mod::eCtrl)) {
			flags.set(Flag::eQuit);
			ret = true;
		}
		if (auto d = state.released(input::Key::eD); d && d->test(input::Mod::eCtrl)) {
			flags.set(Flag::eDebug0);
			ret = true;
		}
		if (reboot(state)) { flags.set(Flag::eReboot); }
		if (close(state)) { flags.set(Flag::eClose); }
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
	window::CreateInfo winInfo;
	winInfo.config.title = "levk demo";
	winInfo.config.size = {1280, 720};
	winInfo.options.centreCursor = true;
	auto eng = Engine::Builder{}.window(std::move(winInfo)).media(&media).addIcon("textures/awesomeface.png")();
	if (!eng) { return false; }
	auto& engine = *eng;
	Flags flags;
	FlagsInput flagsInput(flags);
	engine.service().pushReceiver(&flagsInput);
	Engine::BootInfo bootInfo;
	if constexpr (levk_debug) { bootInfo.device.instance.validation = graphics::Validation::eOn; }
	bootInfo.device.validationLogLevel = LogLevel::info;
	struct Poll : input::EventParser {
		Flags* flags{};
		bool operator()(input::Event const& event) override {
			if (event.type() == input::Event::Type::eClosed) {
				flags->set(Flag::eQuit);
				return true;
			}
			return false;
		}
	};
	Poll poll;
	poll.flags = &flags;
	do {
		flags = {};
		engine.boot(bootInfo);
		auto editor = editor::Instance::make(eng->service());
		SceneManager scenes(engine.service());
		scenes.attach<App>("app", engine.service());
		scenes.open("app");
		DeltaTime dt;
		ktl::kfuture<void> bf;
		ktl::kasync async;
		while (!engine.service().closing()) {
			engine.service().poll(scenes.sceneView(), &poll);
			if (flags.any(Flags(Flag::eQuit, Flag::eReboot))) { break; }
			scenes.tick(++dt);
			scenes.render(RGBA(0x777777ff, RGBA::Type::eAbsolute));
			if (flags.test(Flag::eDebug0) && (!bf.valid() || !bf.busy())) {
				// app.sched().enqueue([]() { ENSURE(false, "test"); });
				// app.sched().enqueue([]() { ENSURE(false, "test2"); });
				auto& ctx = engine.service().context();
				if (auto img = graphics::utils::makeStorage(&ctx.vram(), ctx.lastDrawn().ref())) {
					if (auto file = std::ofstream("shot.ppm", std::ios::out | std::ios::binary)) {
						auto const written = graphics::utils::writePPM(ctx.vram().m_device, *img, file);
						if (written > 0) { logD("Screenshot saved to shot.ppm"); }
					}
				}
			}
			if (flags.test(Flag::eClose)) {
				scenes.close();
				flags.reset(Flag::eClose);
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
		flags.reset(Flag::eQuit);
	} while (flags.test(Flag::eReboot));
	return true;
}
} // namespace le::demo
