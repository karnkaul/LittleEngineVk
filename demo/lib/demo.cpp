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
#include <window/bootstrap.hpp>

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
#include <engine/render/descriptor_helper.hpp>

#include <kt/enum_flags/enumerate_enum.hpp>

namespace le::demo {
using RGBA = graphics::RGBA;

enum class Flag { eClosed, eInit, eTerm, eDebug0 };
using Flags = kt::enum_flags<Flag, u8>;

static void poll(Flags& out_flags, window::EventQueue queue) {
	while (auto e = queue.pop()) {
		switch (e->type) {
		case window::Event::Type::eClose: {
			out_flags.set(Flag::eClosed);
			break;
		}
		case window::Event::Type::eInit: out_flags.set(Flag::eInit); break;
		case window::Event::Type::eTerm: out_flags.set(Flag::eTerm); break;
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
			if (!d.primitives.empty()) {
				bind(1);
				if (d.scissor.set) { cb.setScissor(cast(d.scissor)); }
				for (Primitive const& prim : d.primitives) {
					bind({2, 3});
					ensure(prim.mesh, "Null mesh");
					prim.mesh->draw(cb);
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
			if (!drawable.primitives.empty()) {
				map.set(1).update(0, drawable.model);
				for (Primitive const& prim : drawable.primitives) {
					Material const& mat = prim.material;
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
	TestView(not_null<gui::ViewStack*> parent, not_null<BitmapFont const*> font) : gui::View(parent) {
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
		text.set("click");
		text.set(tf);
		m_button = &push<gui::Widget>(font);
		m_button->m_rect.size = {200.0f, 100.0f};
		m_button->m_styles.quad.at(gui::Status::eHover).Tf = colours::cyan;
		m_button->m_styles.quad.at(gui::Status::eHold).Tf = colours::yellow;
		tf.size = 40U;
		m_button->m_text->set(tf);
		m_button->m_text->set("Button");
		m_button->refresh();
		m_tk = m_button->onClick([this]() { setDestroyed(); });
	}

	TestView(TestView&&) = delete;
	TestView& operator=(TestView&&) = delete;

	gui::Widget* m_button{};
	gui::OnClick::Tk m_tk;
};

class App : public input::Receiver, public SceneRegistry {
  public:
	using SceneRegistry::spawn;

	App(not_null<Engine*> eng) : m_eng(eng), m_drawer(&eng->gfx().boot.vram) {
		// m_manifest.flags().set(AssetManifest::Flag::eImmediate);
		m_manifest.flags().set(AssetManifest::Flag::eOverwrite);
		auto const res = m_manifest.load("demo", &m_tasks);
		ensure(res > 0, "Manifest missing/empty");

		/* custom meshes */ {
			auto cube = m_eng->store().add<graphics::Mesh>("meshes/cube", graphics::Mesh(&eng->gfx().boot.vram));
			cube->construct(graphics::makeCube(0.5f));
			auto cone = m_eng->store().add<graphics::Mesh>("meshes/cone", graphics::Mesh(&eng->gfx().boot.vram));
			cone->construct(graphics::makeCone());
		}
		m_eng->pushReceiver(this);
		eng->m_win->show();

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
			inspector->attach<GFreeCam>("FreeCam");
			inspector->attach<GPlayerController>("PlayerController");
			inspector->attach<GSpringArm>("SpringArm");
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

	decf::spawn_t<SceneNode> spawn(std::string name, Hash meshID, Material const& mat, DrawLayer layer) {
		return spawn(std::move(name), layer, &*m_eng->store().find<graphics::Mesh>(meshID), mat);
	};

	decf::spawn_t<SceneNode> spawn(std::string name, Hash modelID, DrawLayer layer) {
		return spawn(std::move(name), layer, m_eng->store().find<Model>(modelID)->primitives());
	};

	void init1() {
		auto sky_test = m_eng->store().find<graphics::Texture>("cubemaps/test");
		auto skymap = sky_test ? sky_test : m_eng->store().find<graphics::Texture>("cubemaps/sky_dusk");
		auto font = m_eng->store().find<BitmapFont>("fonts/default");
		m_drawer.m_defaults.black = &*m_eng->store().find<graphics::Texture>("textures/black");
		m_drawer.m_defaults.white = &*m_eng->store().find<graphics::Texture>("textures/white");
		m_drawer.m_defaults.cube = &*m_eng->store().find<graphics::Texture>("cubemaps/blank");
		auto& vram = m_eng->gfx().boot.vram;

		m_data.text.make(&vram);
		m_data.text.text.size = 80U;
		m_data.text.text.colour = colours::yellow;
		m_data.text.text.pos = {0.0f, 200.0f, 0.0f};
		// m_data.text.text.align = {-0.5f, 0.5f};
		m_data.text.set(font.get(), "Hi!");

		auto freecam = m_registry.spawn<FreeCam, SpringArm>("freecam");
		m_data.camera = freecam;
		auto& cam = freecam.get<FreeCam>();
		cam.position = {0.0f, 0.5f, 4.0f};
		cam.look({});
		auto& spring = freecam.get<SpringArm>();
		spring.position = cam.position;
		spring.offset = spring.position;

		auto guiStack = spawnStack("gui_root", *m_eng->store().find<DrawLayer>("layers/ui"), &m_eng->gfx().boot.vram);
		m_data.guiStack = guiStack;
		auto& stack = guiStack.get<gui::ViewStack>();
		stack.push<TestView>(&font.get());

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
		spawnSkybox(*m_eng->store().find<DrawLayer>("layers/skybox"), &*skymap);
		{
			Material mat;
			mat.map_Kd = &*m_eng->store().find<graphics::Texture>("textures/container2/diffuse");
			mat.map_Ks = &*m_eng->store().find<graphics::Texture>("textures/container2/specular");
			// d.mat.albedo.diffuse = colours::cyan.toVec3();
			auto player = spawn("player", "meshes/cube", mat, *m_eng->store().find<DrawLayer>("layers/lit"));
			player.get<SceneNode>().position({0.0f, 0.0f, 5.0f});
			m_data.player = player;
			m_registry.attach<PlayerController>(m_data.player);
		}
		{
			auto ent = spawn("prop_1", "meshes/cube", {}, *m_eng->store().find<DrawLayer>("layers/basic"));
			ent.get<SceneNode>().position({-5.0f, -1.0f, -2.0f});
			m_data.entities["prop_1"] = ent;
		}
		{
			auto ent = spawn("prop_2", "meshes/cone", {}, *m_eng->store().find<DrawLayer>("layers/tex"));
			ent.get<SceneNode>().position({1.0f, -2.0f, -3.0f});
		}
		{ spawn("ui_1", *m_eng->store().find<DrawLayer>("layers/ui"), m_data.text.update(*font)); }
		{
			{
				auto ent0 = spawn("model_0_0", "models/plant", *m_eng->store().find<DrawLayer>("layers/lit"));
				// auto ent0 = spawn("model_0_0");
				ent0.get<SceneNode>().position({-2.0f, -1.0f, 2.0f});
				m_data.entities["model_0_0"] = ent0;

				auto ent1 = spawn("model_0_1", "models/plant", *m_eng->store().find<DrawLayer>("layers/lit"));
				auto& node = ent1.get<SceneNode>();
				node.position({-2.0f, -1.0f, 5.0f});
				m_data.entities["model_0_1"] = ent1;
				node.parent(&m_registry.get<SceneNode>(m_data.entities["model_0_0"]));
			}
			if (auto model = m_eng->store().find<Model>("models/teapot")) {
				Primitive& prim = model->primitivesRW().front();
				prim.material.Tf = {0xfc4340ff, RGBA::Type::eAbsolute};
				auto ent0 = spawn("model_1_0", *m_eng->store().find<DrawLayer>("layers/lit"), prim);
				ent0.get<SceneNode>().position({2.0f, -1.0f, 2.0f});
				m_data.entities["model_1_0"] = ent0;
			}
			if (m_eng->store().exists<Model>("models/nanosuit")) {
				auto ent = spawn("model_1", "models/nanosuit", *m_eng->store().find<DrawLayer>("layers/lit"));
				ent.get<SceneNode>().position({-1.0f, -2.0f, -3.0f});
				m_data.entities["model_1"] = ent;
			}
		}
	}

	bool reboot() const noexcept { return m_data.reboot; }

	void tick(Time_s dt) {
		if constexpr (levk_editor) { m_eng->editor().bindNextFrame(this, {m_data.camera}); }

		if (!m_data.unloaded && m_manifest.ready(m_tasks)) {
			if (m_registry.empty()) { init1(); }
			auto guiStack = m_registry.find<gui::ViewStack>(m_data.guiStack);
			if (guiStack) {
				m_eng->update(*guiStack);
				/*auto const& p = m_eng->inputState().cursor.position;
				logD("c: {}, {}", p.x, p.y);*/
				/*static gui::Quad* s_prev = {};
				static Colour s_col;
				if (auto node = guiRoot->leafHit(m_eng->inputState().cursor.position)) {
					if (auto quad = dynamic_cast<gui::Quad*>(node)) {
						if (s_prev != quad) {
							if (s_prev) {
								s_prev->m_material.Tf = s_col;
							}
							s_col = quad->m_material.Tf;
							s_prev = quad;
							quad->m_material.Tf = colours::yellow;
						}
					}
				} else if (s_prev) {
					s_prev->m_material.Tf = s_col;
					s_prev = {};
				}*/
			}
			auto& cam = m_registry.get<FreeCam>(m_data.camera);
			auto& pc = m_registry.get<PlayerController>(m_data.player);
			auto const& state = m_eng->inputFrame().state;
			if (pc.active) {
				auto& node = m_registry.get<SceneNode>(m_data.player);
				m_registry.get<PlayerController>(m_data.player).tick(state, node, dt);
				glm::vec3 const& forward = node.orientation() * -graphics::front;
				cam.position = m_registry.get<SpringArm>(m_data.camera).tick(dt, node.position());
				cam.face(forward);
			} else {
				cam.tick(state, dt, m_eng->desktop());
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

		BitmapText text;
		std::vector<DirLight> dirLights;

		decf::entity camera;
		decf::entity player;
		decf::entity guiStack;
		Hash wire;
		bool reboot = false;
		bool unloaded = {};
	};

	Data m_data;
	scheduler m_tasks;
	AssetManifest m_manifest;
	not_null<Engine*> m_eng;
	Drawer m_drawer;

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

bool run(io::Reader const& reader, ErasedPtr androidApp) {
	dts::g_error_handler = [](std::runtime_error const& err, u64) { ensure(false, err.what()); };
	try {
		window::InstanceBase::CreateInfo winInfo;
		winInfo.config.androidApp = androidApp;
		winInfo.config.title = "levk demo";
		winInfo.config.size = {1280, 720};
		winInfo.options.bCentreCursor = true;
		window::Instance winst(winInfo);
		graphics::Bootstrap::CreateInfo bootInfo;
		bootInfo.instance.extensions = winst.vkInstanceExtensions();
		if constexpr (levk_debug) { bootInfo.instance.validation.mode = graphics::Validation::eOn; }
		bootInfo.instance.validation.logLevel = dl::level::info;
		Engine engine(&winst, {}, &reader);
		Flags flags;
		FlagsInput flagsInput(flags);
		engine.pushReceiver(&flagsInput);
		std::optional<App> app;
		DeltaTime dt;
		while (true) {
			poll(flags, engine.poll(true).residue);
			if (flags.any(Flags(Flag::eClosed) | Flag::eTerm)) { break; }
			if (flags.test(Flag::eInit) || (app && app->reboot())) {
				if (app) {
					logD("Rebooting...");
					app.reset();
				}
				using renderer_t = graphics::Renderer_t<graphics::rtech::fwdOffCb>;
				engine.boot<renderer_t>(bootInfo);
				// engine.boot(bootInfo);
				app.emplace(&engine);
				flags.reset(Flag::eInit);
			}
			if (app) { app->tick(++dt); }
		}
	} catch (std::exception const& e) { logE("exception: {}", e.what()); }
	return true;
}
} // namespace le::demo
