#include <iostream>
#include <core/not_null.hpp>
#include <core/utils/algo.hpp>
#include <core/utils/std_hash.hpp>
#include <core/utils/string.hpp>
#include <dumb_tasks/error_handler.hpp>
#include <dumb_tasks/scheduler.hpp>
#include <engine/assets/asset_list.hpp>
#include <engine/cameras/freecam.hpp>
#include <engine/editor/controls/inspector.hpp>
#include <engine/engine.hpp>
#include <engine/input/control.hpp>
#include <engine/render/model.hpp>
#include <engine/scene/scene_node.hpp>
#include <graphics/common.hpp>
#include <graphics/render/renderers.hpp>
#include <graphics/render/shader_buffer.hpp>
#include <graphics/utils/utils.hpp>

#include <engine/gui/quad.hpp>
#include <engine/gui/text.hpp>
#include <engine/gui/view.hpp>
#include <engine/gui/widget.hpp>
#include <engine/render/bitmap_text.hpp>
#include <engine/scene/list_drawer.hpp>
#include <engine/scene/scene_registry.hpp>
#include <engine/utils/exec.hpp>

#include <core/utils/enumerate.hpp>
#include <engine/assets/asset_manifest.hpp>
#include <engine/gui/widgets/dropdown.hpp>
#include <engine/render/descriptor_helper.hpp>

#include <engine/physics/collision.hpp>
#include <engine/scene/prop_provider.hpp>

namespace le::demo {
using RGBA = graphics::RGBA;

enum class Flag { eClosed, eDebug0 };
using Flags = ktl::enum_flags<Flag, u8>;

static void poll(Flags& out_flags, window::EventQueue queue) {
	while (auto e = queue.pop()) {
		switch (e->type) {
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

struct SpringArm {
	glm::vec3 offset = {};
	glm::vec3 position = {};
	Time_s fixed = 2ms;
	f32 k = 0.5f;
	f32 b = 0.05f;
	struct {
		glm::vec3 velocity = {};
		Time_s ft;
	} data;

	glm::vec3 const& tick(Time_s dt, glm::vec3 const& target) noexcept {
		static constexpr u32 max = 64;
		if (dt.count() > fixed.count() * f32(max)) {
			position = target + offset;
		} else {
			data.ft += dt;
			for (u32 iter = 0; data.ft.count() > 0.0f; ++iter) {
				if (iter == max) {
					position = target + offset;
					break;
				}
				Time_s const diff = data.ft > fixed ? fixed : data.ft;
				data.ft -= diff;
				auto const disp = target + offset - position;
				data.velocity = (1.0f - b) * data.velocity + k * disp;
				position += (diff.count() * data.velocity);
			}
		}
		return position;
	}
};

struct PlayerController {
	glm::quat target = graphics::identity;
	f32 roll = 0.0f;
	f32 maxRoll = glm::radians(45.0f);
	f32 speed = 0.0f;
	f32 maxSpeed = 10.0f;
	bool active = true;

	void tick(input::State const& state, SceneNode& node, Time_s dt) noexcept {
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
		node.orient(target);
		glm::vec3 const dpos = roll * graphics::right + speed * (node.orientation() * -graphics::front);
		node.position(node.position() + 10.0f * dpos * dt.count());
	}
};

class Drawer : public ListDrawer {
  public:
	using Camera = graphics::Camera;

	not_null<graphics::VRAM*> m_vram;

	struct {
		graphics::ShaderBuffer mats;
		graphics::ShaderBuffer lights;
	} m_view;
	struct {
		graphics::Texture const* white = {};
		graphics::Texture const* black = {};
		graphics::Texture const* cube = {};
	} m_defaults;

	Drawer(not_null<graphics::VRAM*> vram) noexcept : m_vram(vram) {}

	void update(decf::registry const& reg, Camera const& cam, glm::vec2 sp, Span<DirLight const> lt, Hash wire) {
		auto lists = populate<DrawListGen3D, DrawListGenUI>(reg);
		write(lists, cam, sp, lt);
		for (auto& list : lists) {
			if (list.pipeline->id() == wire) { list.variant = "wireframe"; }
		}
	}

  private:
	void draw(List const& list, graphics::CommandBuffer cb) const override {
		DescriptorBinder bind(list.pipeline, cb);
		bind(0);
		for (Drawable const& d : list.drawables) {
			if (!d.props.empty()) {
				bind(1);
				if (d.scissor.set) { cb.setScissor(cast(d.scissor)); }
				for (Prop const& prop : d.props) {
					bind({2, 3});
					ensure(prop.mesh, "Null mesh");
					prop.mesh->draw(cb);
				}
			}
		}
	}

	void write(Span<List const> lists, Camera const& cam, glm::vec2 scene, Span<DirLight const> lights) {
		m_view.lights.swap();
		m_view.mats.swap();
		ViewMats const v{cam.view(), cam.perspective(scene), cam.ortho(scene), {cam.position, 1.0f}};
		m_view.mats.write(v);
		if (!lights.empty()) {
			DirLights dl;
			for (std::size_t idx = 0; idx < lights.size() && idx < dl.lights.size(); ++idx) { dl.lights[idx] = lights[idx]; }
			dl.count = std::min((u32)lights.size(), (u32)dl.lights.size());
			m_view.lights.write(dl);
		}
		for (auto& list : lists) { update(list); }
	}

	void update(List const& list) const {
		DescriptorMap map(list.pipeline);
		auto set0 = map.set(0);
		set0.update(0, m_view.mats);
		set0.update(1, m_view.lights);
		for (Drawable const& drawable : list.drawables) {
			if (!drawable.props.empty()) {
				map.set(1).update(0, drawable.model);
				for (Prop const& prop : drawable.props) {
					Material const& mat = prop.material;
					auto set2 = map.set(2);
					set2.update(0, mat.map_Kd && mat.map_Kd->ready() ? *mat.map_Kd : *m_defaults.white);
					set2.update(1, mat.map_d && mat.map_d->ready() ? *mat.map_d : *m_defaults.white);
					set2.update(2, mat.map_Ks && mat.map_Ks->ready() ? *mat.map_Ks : *m_defaults.black);
					map.set(3).update(0, ShadeMat::make(mat));
				}
			}
		}
	}
};

class TestView : public gui::View {
  public:
	TestView(not_null<gui::ViewStack*> parent, std::string name, not_null<BitmapFont const*> font) : gui::View(parent, std::move(name)) {
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
		auto& text = bg.push<gui::Text>(font);
		graphics::TextFactory tf;
		tf.size = 60U;
		text.set("click", tf);
		m_button = &push<gui::Widget>(font);
		m_button->m_rect.size = {200.0f, 100.0f};
		tf.size = 40U;
		m_button->m_text->set("Button", tf);
		m_button->refresh();
		m_tk = m_button->onClick([this](gui::Widget&) { setDestroyed(); });
	}

	TestView(TestView&&) = delete;
	TestView& operator=(TestView&&) = delete;

	gui::Widget* m_button{};
	gui::Widget::OnClick::Tk m_tk;
};
} // namespace le::demo

namespace le::gui {
class Dialogue : public View {
  public:
	struct CreateInfo;
	struct ButtonInfo {
		glm::vec2 size = {150.0f, 35.0f};
		Text::Size textSize = 25U;
		Hash style;
	};

	Dialogue(not_null<gui::ViewStack*> parent, std::string name, not_null<BitmapFont const*> font, CreateInfo const& info);

	[[nodiscard]] Widget::OnClick::Tk addButton(std::string text, Widget::OnClick::Callback const& onClick);

  protected:
	void onUpdate(input::Frame const& frame) override;

	struct Header {
		Widget* title{};
		Widget* close{};
	};
	struct Footer {
		Widget* bg{};
		std::vector<Widget*> buttons;
	};
	struct {
		glm::vec2 prev{};
		glm::vec2 start{};
		glm::vec2 delta{};
		bool moving = false;
	} m_originDelta;
	Header m_header;
	Widget* m_content{};
	Footer m_footer;
	ButtonInfo m_buttonInfo;
	Widget::OnClick::Tk m_closeToken;
	not_null<BitmapFont const*> m_font;
};

struct Dialogue::CreateInfo {
	struct Content {
		std::string text;
		glm::vec2 size = {500.0f, 200.0f};
		Text::Size textSize = 25U;
		Hash style;
	};
	struct Header {
		std::string text;
		f32 height = 50.0f;
		graphics::RGBA background = {0x999999ff, graphics::RGBA::Type::eAbsolute};
		Text::Size textSize = 30U;
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
};

Dialogue::Dialogue(not_null<ViewStack*> parent, std::string name, not_null<BitmapFont const*> font, CreateInfo const& info)
	: View(parent, std::move(name), Block::eBlock), m_buttonInfo(info.buttonInfo), m_font(font) {
	m_content = &push<Widget>(font, info.content.style);
	m_content->m_rect.size = info.content.size;
	m_content->m_style.text.base.size = info.content.textSize;
	m_content->m_text->set(info.content.text);
	m_content->m_interact = false;

	m_header.title = &m_content->push<Widget>(font, info.header.style);
	m_header.title->m_rect.size = {info.content.size.x, info.header.height};
	m_header.title->m_rect.anchor.norm.y = 0.5f;
	m_header.title->m_rect.anchor.offset.y = info.header.height * 0.5f;
	m_header.title->m_style.quad.base.Tf = info.header.background;
	m_header.title->m_style.text.base.size = info.header.textSize;
	m_header.title->m_text->set(info.header.text);
	m_header.title->m_style.quad.reset(InteractStatus::eHover);
	// m_header.title->m_interact = false;
	m_header.close = &m_header.title->push<Widget>(font);
	m_header.close->m_style.quad.base.Tf = colours::red;
	m_header.close->m_style.text.base.colour = colours::white;
	m_header.close->m_style.text.base.size = 20U;
	m_header.close->m_text->set("x");
	m_header.close->m_rect.size = {20.0f, 20.0f};
	m_header.close->m_rect.anchor.norm.x = 0.5f;
	m_header.close->m_rect.anchor.offset.x = -20.0f;
	m_closeToken = m_header.close->onClick([this](Widget&) { setDestroyed(); });

	m_footer.bg = &m_content->push<Widget>(font, info.footer.style);
	m_footer.bg->m_rect.size = {info.content.size.x, info.footer.height};
	m_footer.bg->m_style.quad.base.Tf = info.footer.background;
	m_footer.bg->m_rect.anchor.norm.y = -0.5f;
	m_footer.bg->m_rect.anchor.offset.y = info.footer.height * -0.5f;
	m_footer.bg->m_interact = false;
}

Widget::OnClick::Tk Dialogue::addButton(std::string text, Widget::OnClick::Callback const& onClick) {
	auto& button = m_footer.bg->push<Widget>(m_font, m_buttonInfo.style);
	m_footer.buttons.push_back(&button);
	button.m_style.text.base.size = m_buttonInfo.textSize;
	button.m_rect.anchor.norm.x = -0.5f;
	button.m_rect.size = m_buttonInfo.size;
	button.m_text->set(std::move(text));
	f32 const pad = (m_content->m_rect.size.x - f32(m_footer.buttons.size()) * m_buttonInfo.size.x) / f32(m_footer.buttons.size() + 1);
	f32 offset = pad + m_buttonInfo.size.x * 0.5f;
	for (auto btn : m_footer.buttons) {
		btn->m_rect.anchor.offset.x = offset;
		offset += (pad + m_buttonInfo.size.x);
	}
	return button.onClick(onClick);
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
class App : public input::Receiver, public SceneRegistry {
  public:
	App(not_null<Engine*> eng) : m_eng(eng), m_drawer(&eng->gfx().boot.vram) {
		// auto const io = m_tasks.add_queue();
		// m_tasks.add_agent({io, 0});
		// m_manifest.m_jsonQID = io;
		// m_manifest.flags().set(AssetManifest::Flag::eImmediate);
		m_manifest.flags().set(AssetManifest::Flag::eOverwrite);
		auto const res = m_manifest.load("demo", &m_tasks);
		ensure(res > 0, "Manifest missing/empty");

		m_eng->pushReceiver(this);

		struct GFreeCam : edi::Gadget {
			bool operator()(decf::entity entity, decf::registry& reg) override {
				if (auto freecam = reg.find<FreeCam>(entity)) {
					edi::Text title("FreeCam");
					edi::TWidget<f32>("Speed", freecam->m_params.xz_speed);
					return true;
				}
				return false;
			}
		};
		struct GPlayerController : edi::Gadget {
			bool operator()(decf::entity entity, decf::registry& reg) override {
				if (auto pc = reg.find<PlayerController>(entity)) {
					edi::Text title("PlayerController");
					edi::TWidget<bool>("Active", pc->active);
					return true;
				}
				return false;
			}
		};
		struct GSpringArm : edi::Gadget {
			bool operator()(decf::entity entity, decf::registry& reg) override {
				if (auto sa = reg.find<SpringArm>(entity)) {
					edi::Text title("SpringArm");
					edi::TWidget<f32>("k", sa->k, 0.01f);
					edi::TWidget<f32>("b", sa->b, 0.001f);
					return true;
				}
				return false;
			}
		};
		if (auto inspector = Services::locate<edi::Inspector>(false)) {
			inspector->attach<GFreeCam>();
			inspector->attach<GPlayerController>();
			inspector->attach<GSpringArm>();
		}
	}

	bool block(input::State const& state) override {
		if (m_controls.editor(state)) { m_eng->editor().toggle(); }
		if (m_controls.wireframe(state)) { m_data.wire = m_data.wire == Hash() ? "pipelines/lit" : Hash(); }
		if (m_controls.reboot(state)) { m_data.reboot = true; }
		if (m_controls.unload(state)) {
			m_data.unloaded = true;
			m_registry.clear();
			logI("{} unloaded", m_manifest.unload("demo", m_tasks));
		}
		return false;
	}

	void init1() {
		auto sky_test = m_eng->store().find<graphics::Texture>("cubemaps/test");
		auto skymap = sky_test ? sky_test : m_eng->store().find<graphics::Texture>("cubemaps/sky_dusk");
		auto font = m_eng->store().find<BitmapFont>("fonts/default");
		m_drawer.m_defaults.black = &*m_eng->store().find<graphics::Texture>("textures/black");
		m_drawer.m_defaults.white = &*m_eng->store().find<graphics::Texture>("textures/white");
		m_drawer.m_defaults.cube = &*m_eng->store().find<graphics::Texture>("cubemaps/blank");
		auto& vram = m_eng->gfx().boot.vram;
		auto coll = spawn<Collision>("collision", "layers/wireframe");
		auto& collision = coll.get<Collision>();
		m_data.collision = coll;

		m_data.text = Text2D(&*font, &vram);
		m_data.text.factory().size = 80U;
		m_data.text.factory().colour = colours::yellow;
		m_data.text.factory().pos = {0.0f, 200.0f, 0.0f};
		// m_data.text.text.align = {-0.5f, 0.5f};
		m_data.text.set("Hi!");

		auto freecam = m_registry.spawn<FreeCam, SpringArm>("freecam");
		m_data.camera = freecam;
		auto& cam = freecam.get<FreeCam>();
		cam.position = {0.0f, 0.5f, 4.0f};
		cam.look({});
		auto& spring = freecam.get<SpringArm>();
		spring.position = cam.position;
		spring.offset = spring.position;

		auto guiStack = spawn<gui::ViewStack>("gui_root", "layers/ui", &m_eng->gfx().boot.vram);
		m_data.guiStack = guiStack;
		auto& stack = guiStack.get<gui::ViewStack>();
		[[maybe_unused]] auto& testView = stack.push<TestView>("test_view", &font.get());
		gui::Dropdown::CreateInfo dci;
		dci.flexbox.background.Tf = RGBA(0x888888ff, RGBA::Type::eAbsolute);
		// dci.quadStyle.at(gui::InteractStatus::eHover).Tf = colours::cyan;
		dci.textSize = 30U;
		dci.options = {"zero", "one", "two", "/bthree", "four"};
		dci.selected = 2;
		auto& dropdown = testView.push<gui::Dropdown>(&font.get(), std::move(dci));
		dropdown.m_rect.anchor.offset = {-300.0f, -50.0f};
		gui::Dialogue::CreateInfo gdci;
		gdci.header.text = "Dialogue";
		gdci.content.text = "Content goes here";
		auto& dialogue = stack.push<gui::Dialogue>("test_dialogue", &font.get(), gdci);
		m_data.btnTkns.push_back(dialogue.addButton("OK", [&dialogue](gui::Widget&) { dialogue.setDestroyed(); }));
		m_data.btnTkns.push_back(dialogue.addButton("Cancel", [&dialogue](gui::Widget&) { dialogue.setDestroyed(); }));
		m_drawer.m_view.mats = graphics::ShaderBuffer(vram, {});
		{
			graphics::ShaderBuffer::CreateInfo info;
			info.type = vk::DescriptorType::eStorageBuffer;
			m_drawer.m_view.lights = graphics::ShaderBuffer(vram, {});
		}
		DirLight l0, l1;
		l0.direction = {-graphics::up, 0.0f};
		l1.direction = {-graphics::front, 0.0f};
		l0.albedo = Albedo::make(colours::cyan, {0.2f, 0.5f, 0.3f, 0.0f});
		l1.albedo = Albedo::make(colours::white, {0.4f, 1.0f, 0.8f, 0.0f});
		m_data.dirLights = {l0, l1};
		spawn<Skybox>("skybox", "layers/skybox", &*skymap);
		{
			Material mat;
			mat.map_Kd = &*m_eng->store().find<graphics::Texture>("textures/container2/diffuse");
			mat.map_Ks = &*m_eng->store().find<graphics::Texture>("textures/container2/specular");
			// d.mat.albedo.diffuse = colours::cyan.toVec3();
			auto player = spawnMesh("player", "meshes/cube", "layers/lit", mat);
			player.get<SceneNode>().position({0.0f, 0.0f, 5.0f});
			m_data.player = player;
			m_registry.attach<PlayerController>(m_data.player);
			auto coll = collision.add({});
			m_colTk = coll.onCollide([](Collision::Collider) { logD("Collided!"); });
			m_colID = coll.m_id;
		}
		{
			auto ent = spawnProp<graphics::Mesh>("prop_1", "meshes/cube", "layers/basic");
			ent.get<SceneNode>().position({-5.0f, -1.0f, -2.0f});
			m_data.entities["prop_1"] = ent;
		}
		{
			auto ent = spawnProp<graphics::Mesh>("prop_2", "meshes/cone", "layers/tex");
			ent.get<SceneNode>().position({1.0f, -2.0f, -3.0f});
		}
		// { spawn("ui_1", *m_eng->store().find<DrawLayer>("layers/ui"), m_data.text.prop(*font)); }
		{ spawnProp<Text2D>("text_2d", m_data.text, "layers/ui"); }
		{
			{
				auto ent0 = spawnProp<Model>("model_0_0", "models/plant", "layers/lit");
				ent0.get<SceneNode>().position({-2.0f, -1.0f, 2.0f});
				m_data.entities["model_0_0"] = ent0;

				auto ent1 = spawnProp<Model>("model_0_1", "models/plant", "layers/lit");
				auto& node = ent1.get<SceneNode>();
				node.position({-2.0f, -1.0f, 5.0f});
				m_data.entities["model_0_1"] = ent1;
				node.parent(&m_registry.get<SceneNode>(m_data.entities["model_0_0"]));
			}
			if (auto model = m_eng->store().find<Model>("models/teapot")) {
				Prop& prop = model->propsRW().front();
				prop.material.Tf = {0xfc4340ff, RGBA::Type::eAbsolute};
				auto ent0 = spawnProp<Model>("model_1_0", "models/teapot", "layers/lit");
				ent0.get<SceneNode>().position({2.0f, -1.0f, 2.0f});
				m_data.entities["model_1_0"] = ent0;
			}
			if (m_eng->store().exists<Model>("models/nanosuit")) {
				auto ent = spawnProp<Model>("model_1", "models/nanosuit", "layers/lit");
				ent.get<SceneNode>().position({-1.0f, -2.0f, -3.0f});
				m_data.entities["model_1"] = ent;
			}
		}
		{
			Material mat;
			mat.Tf = colours::yellow;
			auto node = spawnMesh("collision/cube", "meshes/cube", "layers/basic", mat);
			node.get<SceneNode>().scale(2.0f);
			collision.add({glm::vec3(2.0f)});
		}
	}

	bool reboot() const noexcept { return m_data.reboot; }

	void tick(Time_s dt) {
		if constexpr (levk_editor) { m_eng->editor().bindNextFrame(this, {m_data.camera}); }

		update();
		if (!m_data.unloaded && m_manifest.ready(m_tasks)) {
			auto collision = m_registry.find<Collision>(m_data.collision);
			if (m_registry.empty()) { init1(); }
			auto& cam = m_registry.get<FreeCam>(m_data.camera);
			auto& pc = m_registry.get<PlayerController>(m_data.player);
			auto const& state = m_eng->inputFrame().state;
			if (pc.active) {
				auto& node = m_registry.get<SceneNode>(m_data.player);
				pc.tick(state, node, dt);
				glm::vec3 const& forward = node.orientation() * -graphics::front;
				cam.position = m_registry.get<SpringArm>(m_data.camera).tick(dt, node.position());
				cam.face(forward);
				if (collision) { collision->find(m_colID)->position() = node.position(); }
			} else {
				cam.tick(state, dt, &m_eng->window());
			}
			m_registry.get<SceneNode>(m_data.entities["prop_1"]).rotate(glm::radians(360.0f) * dt.count(), graphics::up);
			if (auto node = m_registry.find<SceneNode>(m_data.entities["model_0_0"])) { node->rotate(glm::radians(-75.0f) * dt.count(), graphics::up); }
			if (auto node = m_registry.find<SceneNode>(m_data.entities["model_1_0"])) {
				static glm::quat s_axis = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
				s_axis = glm::rotate(s_axis, glm::radians(45.0f) * dt.count(), graphics::front);
				node->rotate(glm::radians(90.0f) * dt.count(), glm::normalize(s_axis * graphics::up));
			}
		}
		// draw
		render();
	}

	void render() {
		if (m_eng->nextFrame()) {
			// write / update
			if (auto cam = m_registry.find<FreeCam>(m_data.camera)) { m_drawer.update(m_registry, *cam, m_eng->sceneSpace(), m_data.dirLights, m_data.wire); }
			// draw
			m_eng->draw(m_drawer, RGBA(0x777777ff, RGBA::Type::eAbsolute));
		}
	}

  private:
	struct Data {
		std::unordered_map<Hash, decf::entity> entities;

		Text2D text;
		std::vector<DirLight> dirLights;
		std::vector<gui::Widget::OnClick::Tk> btnTkns;

		decf::entity camera;
		decf::entity player;
		decf::entity guiStack;
		decf::entity collision;
		Hash wire;
		bool reboot = false;
		bool unloaded = {};
	};

	Data m_data;
	scheduler m_tasks;
	AssetManifest m_manifest;
	not_null<Engine*> m_eng;
	Drawer m_drawer;
	Collision::ID m_colID{};
	Collision::OnCollide::Tk m_colTk;

	struct {
		input::Trigger editor = {input::Key::eE, input::Action::ePressed, input::Mod::eControl};
		input::Trigger wireframe = {input::Key::eP, input::Action::ePressed, input::Mod::eControl};
		input::Trigger reboot = {input::Key::eR, input::Action::ePressed, input::Mods::make(input::Mod::eControl, input::Mod::eShift)};
		input::Trigger unload = {input::Key::eU, input::Action::ePressed, input::Mods::make(input::Mod::eControl, input::Mod::eShift)};
	} m_controls;
};

struct FlagsInput : input::Receiver {
	Flags& flags;

	FlagsInput(Flags& flags) : flags(flags) {}

	bool block(input::State const& state) override {
		bool ret = false;
		if (auto key = state.released(input::Key::eW); key && key->mods[input::Mod::eControl]) {
			flags.set(Flag::eClosed);
			ret = true;
		}
		if (auto key = state.pressed(input::Key::eD); key && key->mods[input::Mod::eControl]) {
			flags.set(Flag::eDebug0);
			ret = true;
		}
		return ret;
	}
};

bool run(io::Reader const& reader) {
	dts::g_error_handler = [](std::runtime_error const& err, u64) { ensure(false, err.what()); };
	Engine::CreateInfo eci;
	eci.winInfo.config.title = "levk demo";
	eci.winInfo.config.size = {1280, 720};
	eci.winInfo.options.centreCursor = true;
	Engine engine(eci, &reader);
	if (!engine.bootReady()) { return false; }
	Flags flags;
	FlagsInput flagsInput(flags);
	engine.pushReceiver(&flagsInput);
	bool reboot = false;
	Engine::Boot::CreateInfo bootInfo;
	if constexpr (levk_debug) { bootInfo.instance.validation.mode = graphics::Validation::eOn; }
	bootInfo.instance.validation.logLevel = dl::level::info;
	do {
		using renderer_t = graphics::Renderer_t<graphics::rtech::fwdOffCb>;
		// engine.boot(bootInfo);
		engine.boot<renderer_t>(bootInfo);
		App app(&engine);
		DeltaTime dt;
		std::optional<window::Instance> test;
		while (!engine.closing()) {
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
		}
	} while (reboot);
	return true;
}
} // namespace le::demo
