#include <iostream>
#include <core/not_null.hpp>
#include <core/utils/algo.hpp>
#include <core/utils/std_hash.hpp>
#include <core/utils/string.hpp>
#include <dumb_tasks/error_handler.hpp>
#include <dumb_tasks/scheduler.hpp>
#include <engine/assets/asset_list.hpp>
#include <engine/assets/asset_loaders.hpp>
#include <engine/camera.hpp>
#include <engine/cameras/freecam.hpp>
#include <engine/editor/controls/inspector.hpp>
#include <engine/engine.hpp>
#include <engine/input/control.hpp>
#include <engine/render/model.hpp>
#include <engine/scene/scene_drawer.hpp>
#include <engine/scene/scene_node.hpp>
#include <graphics/common.hpp>
#include <graphics/shader_buffer.hpp>
#include <graphics/utils/utils.hpp>
#include <window/bootstrap.hpp>

#include <engine/gui/quad.hpp>
#include <engine/gui/text.hpp>
#include <engine/gui/view.hpp>
#include <engine/gui/widget.hpp>
#include <engine/render/bitmap_text.hpp>
#include <engine/utils/exec.hpp>

namespace le::demo {
enum class Flag { eRecreated, eResized, ePaused, eClosed, eInit, eTerm, eDebug0, eCOUNT_ };
using Flags = kt::enum_flags<Flag>;

static void poll(Flags& out_flags, window::EventQueue queue) {
	while (auto e = queue.pop()) {
		switch (e->type) {
		case window::Event::Type::eClose: {
			out_flags.set(Flag::eClosed);
			break;
		}
		case window::Event::Type::eSuspend: {
			out_flags[Flag::ePaused] = e->payload.bSet;
			break;
		}
		case window::Event::Type::eResize: {
			auto const& resize = e->payload.resize;
			if (resize.bFramebuffer) { out_flags.set(Flag::eResized); }
			break;
		}
		case window::Event::Type::eInit: out_flags.set(Flag::eInit); break;
		case window::Event::Type::eTerm: out_flags.set(Flag::eTerm); break;
		default: break;
		}
	}
}

using namespace dts;

struct TaskErr : error_handler_t {
	void operator()(std::runtime_error const& err, u64) const override { ENSURE(false, err.what()); }
};
TaskErr g_taskErr;

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

class DrawDispatch {
  public:
	not_null<graphics::VRAM*> m_vram;

	struct {
		graphics::ShaderBuffer mats;
		graphics::ShaderBuffer lights;
	} m_view;
	struct {
		graphics::Texture const* white = {};
		graphics::Texture const* black = {};
	} m_defaults;

	struct SetBind {
		u32 set;
		u32 bind;
		bool valid;

		SetBind(graphics::ShaderInput const& si, u32 s, u32 b) : set(s), bind(b), valid(si.contains(s, b)) {}

		explicit operator bool() const noexcept { return valid; }
	};

	DrawDispatch(not_null<graphics::VRAM*> vram) noexcept : m_vram(vram) {}

	void write(Camera const& cam, glm::vec2 fb, Span<DirLight const> lights, Span<SceneDrawer::Group const> groups) {
		ViewMats const v{cam.view(), cam.perspective(fb), cam.ortho(fb), {cam.position, 1.0f}};
		m_view.mats.write(v);
		if (!lights.empty()) {
			DirLights dl;
			for (std::size_t idx = 0; idx < lights.size() && idx < dl.lights.size(); ++idx) { dl.lights[idx] = lights[idx]; }
			dl.count = std::min((u32)lights.size(), (u32)dl.lights.size());
			m_view.lights.write(dl);
		}
		for (auto& group : groups) { update(group); }
	}

	void update(SceneDrawer::Group const& group) const {
		auto& si = group.group.pipeline->shaderInput();
		si.update(m_view.mats, 0, 0, 0);
		if (group.group.order >= 0 && si.contains(0, 1)) { si.update(m_view.lights, 0, 1, 0); }
		auto const sb10 = SetBind(si, 1, 0);
		auto const sb20 = SetBind(si, 2, 0);
		auto const sb21 = SetBind(si, 2, 1);
		auto const sb22 = SetBind(si, 2, 2);
		auto const sb30 = SetBind(si, 3, 0);
		std::size_t primIdx = 0;
		std::size_t itemIdx = 0;
		for (SceneDrawer::Item const& item : group.items) {
			if (!item.primitives.empty()) {
				if (sb10) {
					graphics::Buffer const buf = m_vram->makeBO(item.model, vk::BufferUsageFlagBits::eUniformBuffer);
					si.update(buf, sb10.set, sb10.bind, itemIdx);
				}
				++itemIdx;
				for (Primitive const& prim : item.primitives) {
					Material const& mat = prim.material;
					if (group.group.order < 0) {
						ENSURE(mat.map_Kd, "Null cubemap");
						si.update(*mat.map_Kd, 0, 1, primIdx);
					}
					if (sb20) { si.update(mat.map_Kd ? *mat.map_Kd : *m_defaults.white, sb20.set, sb20.bind, primIdx); }
					if (sb21) { si.update(mat.map_d ? *mat.map_d : *m_defaults.white, sb21.set, sb21.bind, primIdx); }
					if (sb22) { si.update(mat.map_Ks ? *mat.map_Ks : *m_defaults.black, sb22.set, sb22.bind, primIdx); }
					if (sb30) {
						auto const sm = ShadeMat::make(mat);
						graphics::Buffer const buf = m_vram->makeBO(sm, vk::BufferUsageFlagBits::eUniformBuffer);
						si.update(buf, sb30.set, sb30.bind, primIdx);
					}
					++primIdx;
				}
			}
		}
	}

	void swap() {
		m_view.mats.swap();
		m_view.lights.swap();
	}

	void draw(graphics::CommandBuffer const& cb, SceneDrawer::Group const& group) const {
		graphics::Pipeline const& pipe = *group.group.pipeline;
		pipe.bindSet(cb, 0, 0);
		std::size_t itemIdx = 0;
		std::size_t primIdx = 0;
		for (SceneDrawer::Item const& d : group.items) {
			if (!d.primitives.empty()) {
				pipe.bindSet(cb, 1, itemIdx++);
				if (d.scissor) { cb.setScissor(*d.scissor); }
				for (Primitive const& prim : d.primitives) {
					pipe.bindSet(cb, {2, 3}, primIdx++);
					ENSURE(prim.mesh, "Null mesh");
					prim.mesh->draw(cb);
				}
			}
		}
	}
};

class TestView : public gui::View {
  public:
	TestView(not_null<gui::ViewStack*> parent, not_null<BitmapFont const*> font) : gui::View(parent) {
		graphics::VRAM* vram = parent->m_vram;
		m_canvas.size.value = {1280.0f, 720.0f};
		m_canvas.size.unit = gui::Unit::eAbsolute;
		auto& bg = push<gui::Quad>(vram);
		bg.m_rect.size = {200.0f, 100.0f};
		bg.m_rect.anchor.norm = {-0.25f, 0.25f};
		bg.m_material.Tf = colours::cyan;
		auto& centre = bg.push<gui::Quad>(vram);
		centre.m_rect.size = {50.0f, 50.0f};
		centre.m_material.Tf = colours::red;
		auto& dot = centre.push<gui::Quad>(vram);
		dot.offset({30.0f, 20.0f});
		dot.m_material.Tf = Colour(0x333333ff);
		dot.m_rect.anchor.norm = {-0.5f, -0.5f};
		auto& topLeft = bg.push<gui::Quad>(vram);
		topLeft.m_rect.anchor.norm = {-0.5f, 0.5f};
		topLeft.offset({25.0f, 25.0f}, {1.0f, -1.0f});
		topLeft.m_material.Tf = colours::magenta;
		auto& text = bg.push<gui::Text>(vram, font);
		graphics::TextFactory tf;
		tf.size = 60U;
		text.set("click");
		text.set(tf);
		m_button = &push<gui::Widget>(vram, font);
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

class App : public input::Receiver {
  public:
	App(not_null<Engine*> eng, io::Reader const& reader) : m_eng(eng), m_drawDispatch(&eng->gfx().boot.vram) {
		dts::g_error_handler = &g_taskErr;
		auto loadShader = [this](std::string_view id, io::Path v, io::Path f) {
			AssetLoadData<graphics::Shader> shaderLD{&m_eng->gfx().boot.device};
			shaderLD.name = id;
			shaderLD.shaderPaths[graphics::Shader::Type::eVertex] = std::move(v);
			shaderLD.shaderPaths[graphics::Shader::Type::eFragment] = std::move(f);
			return shaderLD;
		};
		using PCI = graphics::Pipeline::CreateInfo;
		auto loadPipe = [this](std::string_view id, Hash shaderID, graphics::PFlags flags = {}, std::optional<PCI> pci = std::nullopt) {
			AssetLoadData<graphics::Pipeline> pipelineLD{&m_eng->gfx().context};
			pipelineLD.name = id;
			pipelineLD.shaderID = shaderID;
			pipelineLD.info = pci;
			pipelineLD.flags = flags;
			return pipelineLD;
		};
		m_store.resources().reader(&reader);
		m_store.add("samplers/default", graphics::Sampler{&eng->gfx().boot.device, graphics::Sampler::info({vk::Filter::eLinear, vk::Filter::eLinear})});
		{
			AssetList<Model> models;
			AssetLoadData<Model> ald(&m_eng->gfx().boot.vram);
			ald.modelID = "models/plant";
			ald.jsonID = "models/plant/plant.json";
			ald.samplerID = "samplers/default";
			models.add("models/plant", std::move(ald));

			ald.jsonID = "models/teapot/teapot.json";
			ald.modelID = "models/teapot";
			models.add("models/teapot", std::move(ald));

			ald.jsonID = "models/test/nanosuit/nanosuit.json";
			if (m_store.resources().reader().present(ald.jsonID)) {
				ald.modelID = "models/nanosuit";
				models.add("models/nanosuit", std::move(ald));
			}
			m_data.loader.stage(m_store, models, m_tasks);
		}

		AssetLoadData<BitmapFont> fld(&m_eng->gfx().boot.vram);
		fld.jsonID = "fonts/default/default.json";
		fld.samplerID = "samplers/default";
		m_data.loader.stage(m_store, AssetList<BitmapFont>{{{"fonts/default", std::move(fld)}}}, m_tasks);
		{
			graphics::Geometry gcube = graphics::makeCube(0.5f);
			auto const skyCubeI = gcube.indices;
			auto const skyCubeV = gcube.positions();
			auto cube = m_store.add<graphics::Mesh>("meshes/cube", graphics::Mesh(&eng->gfx().boot.vram));
			cube->construct(gcube);
			auto cone = m_store.add<graphics::Mesh>("meshes/cone", graphics::Mesh(&eng->gfx().boot.vram));
			cone->construct(graphics::makeCone());
			auto skycube = m_store.add<graphics::Mesh>("skycube", graphics::Mesh(&eng->gfx().boot.vram));
			skycube->construct(Span<glm::vec3 const>(skyCubeV), skyCubeI);
		}

		{
			AssetList<graphics::Shader> shaders;
			shaders.add("shaders/basic", loadShader("shaders/basic", "shaders/basic.vert", "shaders/basic.frag"));
			shaders.add("shaders/tex", loadShader("shaders/tex", "shaders/basic.vert", "shaders/tex.frag"));
			shaders.add("shaders/lit", loadShader("shaders/lit", "shaders/lit.vert", "shaders/lit.frag"));
			shaders.add("shaders/ui", loadShader("shaders/ui", "shaders/ui.vert", "shaders/ui.frag"));
			shaders.add("shaders/skybox", loadShader("shaders/skybox", "shaders/skybox.vert", "shaders/skybox.frag"));
			auto load_shaders = m_data.loader.stage(m_store, shaders, m_tasks);

			AssetList<graphics::Pipeline> pipes;
			static PCI pci_skybox = eng->gfx().context.pipeInfo();
			pci_skybox.fixedState.depthStencilState.depthWriteEnable = false;
			pci_skybox.fixedState.vertexInput = eng->gfx().context.vertexInput({0, sizeof(glm::vec3), {{vk::Format::eR32G32B32Sfloat, 0}}});
			pipes.add("pipelines/basic", loadPipe("pipelines/basic", "shaders/basic", graphics::PFlags::inverse()));
			pipes.add("pipelines/tex", loadPipe("pipelines/tex", "shaders/tex", graphics::PFlags::inverse()));
			pipes.add("pipelines/lit", loadPipe("pipelines/lit", "shaders/lit", graphics::PFlags::inverse()));
			graphics::PFlags ui = graphics::PFlags::inverse();
			ui.reset(graphics::PFlags(graphics::PFlag::eDepthTest) | graphics::PFlag::eDepthWrite);
			pipes.add("pipelines/ui", loadPipe("pipelines/ui", "shaders/ui", ui));
			pipes.add("pipelines/skybox", loadPipe("pipelines/skybox", "shaders/skybox", {}, pci_skybox));
			m_data.loader.stage(m_store, pipes, m_tasks, load_shaders);
		}

		AssetList<graphics::Texture> texList;
		AssetLoadData<graphics::Texture> textureLD{&eng->gfx().boot.vram};
		textureLD.samplerID = "samplers/default";
		textureLD.prefix = "skyboxes/sky_dusk";
		textureLD.ext = ".jpg";
		textureLD.imageIDs = {"right", "left", "up", "down", "front", "back"};
		texList.add("cubemaps/sky_dusk", std::move(textureLD));
		textureLD.prefix.clear();
		textureLD.ext.clear();
		textureLD.imageIDs = {"textures/container2.png"};
		texList.add("textures/container2/diffuse", std::move(textureLD));
		textureLD.prefix.clear();
		textureLD.ext.clear();
		textureLD.imageIDs = {"textures/container2_specular.png"};
		texList.add("textures/container2/specular", std::move(textureLD));
		textureLD.imageIDs.clear();
		textureLD.bitmap.bytes = graphics::utils::bitmapPx({0xff0000ff});
		textureLD.bitmap.size = {1, 1};
		textureLD.rawBytes = true;
		texList.add("textures/red", std::move(textureLD));
		textureLD.bitmap.bytes = graphics::utils::bitmapPx({0x000000ff});
		texList.add("textures/black", std::move(textureLD));
		textureLD.bitmap.bytes = graphics::utils::bitmapPx({0xffffffff});
		texList.add("textures/white", std::move(textureLD));
		textureLD.bitmap.bytes = graphics::utils::bitmapPx({0x0});
		texList.add("textures/blank", std::move(textureLD));
		m_data.loader.stage(m_store, texList, m_tasks);
		m_eng->pushReceiver(this);
		eng->m_win->show();

		struct GFreeCam : edi::Gadget {
			bool operator()(decf::entity_t entity, decf::registry_t& reg) override {
				if (auto freecam = reg.find<FreeCam>(entity)) {
					edi::Text title("FreeCam");
					edi::TWidget<f32>("Speed", freecam->m_params.xz_speed);
					return true;
				}
				return false;
			}
		};
		struct GPlayerController : edi::Gadget {
			bool operator()(decf::entity_t entity, decf::registry_t& reg) override {
				if (auto pc = reg.find<PlayerController>(entity)) {
					edi::Text title("PlayerController");
					edi::TWidget<bool>("Active", pc->active);
					return true;
				}
				return false;
			}
		};
		struct GSpringArm : edi::Gadget {
			bool operator()(decf::entity_t entity, decf::registry_t& reg) override {
				if (auto sa = reg.find<SpringArm>(entity)) {
					edi::Text title("SpringArm");
					edi::TWidget<f32>("k", sa->k, 0.01f);
					edi::TWidget<f32>("b", sa->b, 0.001f);
					return true;
				}
				return false;
			}
		};
		edi::Inspector::attach<GFreeCam>("FreeCam");
		edi::Inspector::attach<GPlayerController>("PlayerController");
		edi::Inspector::attach<GSpringArm>("SpringArm");
	}

	bool block(input::State const& state) override {
		if (state.focus == input::Focus::eGained) { m_store.update(); }
		if (m_controls.editor(state)) { Editor::s_engaged = !Editor::s_engaged; }
		return false;
	}

	decf::spawn_t<SceneNode> spawn(std::string name) {
		auto ret = m_data.registry.spawn<SceneNode>(name, &m_data.root);
		ret.get<SceneNode>().entity(ret);
		return ret;
	}

	decf::spawn_t<SceneNode> spawn(std::string name, DrawGroup const& group, Span<Primitive const> primitives) {
		auto ret = spawn(std::move(name));
		SceneDrawer::attach(m_data.registry, ret, group, primitives);
		return ret;
	};

	decf::spawn_t<SceneNode> spawn(std::string name, Hash meshID, Material const& mat, DrawGroup const& group) {
		auto m = m_store.get<graphics::Mesh>(meshID);
		auto ret = spawn(std::move(name));
		SceneDrawer::attach(m_data.registry, ret, group, Primitive{mat, &*m});
		return ret;
	};

	decf::spawn_t<SceneNode> spawn(std::string name, Hash modelID, DrawGroup const& group) {
		auto m = m_store.get<Model>(modelID);
		auto ret = spawn(std::move(name));
		SceneDrawer::attach(m_data.registry, ret, group, m->primitives());
		return ret;
	};

	void init1() {
		auto pipe_test = m_store.find<graphics::Pipeline>("pipelines/basic");
		auto pipe_testTex = m_store.find<graphics::Pipeline>("pipelines/tex");
		auto pipe_testLit = m_store.find<graphics::Pipeline>("pipelines/lit");
		auto pipe_ui = m_store.find<graphics::Pipeline>("pipelines/ui");
		auto pipe_sky = m_store.find<graphics::Pipeline>("pipelines/skybox");
		auto skymap = m_store.get<graphics::Texture>("cubemaps/sky_dusk");
		auto font = m_store.get<BitmapFont>("fonts/default");
		m_drawDispatch.m_defaults.black = &m_store.get<graphics::Texture>("textures/black").get();
		m_drawDispatch.m_defaults.white = &m_store.get<graphics::Texture>("textures/white").get();
		auto& vram = m_eng->gfx().boot.vram;

		m_data.text.create(&vram);
		m_data.text.text.size = 80U;
		m_data.text.text.colour = colours::yellow;
		m_data.text.text.pos = {0.0f, 200.0f, 0.0f};
		// m_data.text.text.align = {-0.5f, 0.5f};
		m_data.text.set(font.get(), "Hi!");

		auto freecam = m_data.registry.spawn<FreeCam, SpringArm>("freecam");
		m_data.camera = freecam;
		auto& cam = freecam.get<FreeCam>();
		cam.position = {0.0f, 0.5f, 4.0f};
		cam.look({});
		auto& spring = freecam.get<SpringArm>();
		spring.position = cam.position;
		spring.offset = spring.position;

		m_data.groups["sky"] = DrawGroup{&pipe_sky->get(), -10};
		m_data.groups["test"] = DrawGroup{&pipe_test->get(), 0};
		m_data.groups["test_tex"] = DrawGroup{&pipe_testTex->get(), 0};
		m_data.groups["test_lit"] = DrawGroup{&pipe_testLit->get(), 0};
		m_data.groups["ui"] = DrawGroup{&pipe_ui->get(), 10};

		auto guiStack = m_data.registry.spawn<gui::ViewStack>("gui_root", &m_eng->gfx().boot.vram);
		m_data.guiStack = guiStack;
		m_data.registry.attach<DrawGroup>(guiStack, m_data.groups["ui"]);
		auto& stack = guiStack.get<gui::ViewStack>();
		stack.push<TestView>(&font.get());

		m_drawDispatch.m_view.mats = graphics::ShaderBuffer(vram, {});
		{
			graphics::ShaderBuffer::CreateInfo info;
			info.type = vk::DescriptorType::eStorageBuffer;
			m_drawDispatch.m_view.lights = graphics::ShaderBuffer(vram, {});
		}
		DirLight l0, l1;
		l0.direction = {-graphics::up, 0.0f};
		l1.direction = {-graphics::front, 0.0f};
		l0.albedo = Albedo::make(colours::cyan, {0.2f, 0.5f, 0.3f, 0.0f});
		l1.albedo = Albedo::make(colours::white, {0.4f, 1.0f, 0.8f, 0.0f});
		m_data.dirLights = {l0, l1};
		{
			Material mat;
			mat.map_Kd = &*skymap;
			spawn("skybox", "skycube", mat, m_data.groups["sky"]);
		}
		{
			Material mat;
			mat.map_Kd = &*m_store.get<graphics::Texture>("textures/container2/diffuse");
			mat.map_Ks = &*m_store.get<graphics::Texture>("textures/container2/specular");
			// d.mat.albedo.diffuse = colours::cyan.toVec3();
			auto player = spawn("player", "meshes/cube", mat, m_data.groups["test_lit"]);
			player.get<SceneNode>().position({0.0f, 0.0f, 5.0f});
			m_data.player = player;
			// m_data.player = spawn("player");
			m_data.registry.attach<PlayerController>(m_data.player);
		}
		{
			auto ent = spawn("prop_1", "meshes/cube", {}, m_data.groups["test"]);
			ent.get<SceneNode>().position({-5.0f, -1.0f, -2.0f});
			m_data.entities["prop_1"] = ent;
		}
		{
			auto ent = spawn("prop_2", "meshes/cone", {}, m_data.groups["test_tex"]);
			ent.get<SceneNode>().position({1.0f, -2.0f, -3.0f});
		}
		{ spawn("ui_1", m_data.groups["ui"], m_data.text.primitive(*font)); }
		{
			{
				auto ent0 = spawn("model_0_0", "models/plant", m_data.groups["test_lit"]);
				// auto ent0 = spawn("model_0_0");
				ent0.get<SceneNode>().position({-2.0f, -1.0f, 2.0f});
				m_data.entities["model_0_0"] = ent0;

				auto ent1 = spawn("model_0_1", "models/plant", m_data.groups["test_lit"]);
				auto& node = ent1.get<SceneNode>();
				node.position({-2.0f, -1.0f, 5.0f});
				m_data.entities["model_0_1"] = ent1;
				node.parent(&m_data.registry.get<SceneNode>(m_data.entities["model_0_0"]));
			}
			if (auto model = m_store.find<Model>("models/teapot")) {
				Primitive prim = model->get().primitives().front();
				prim.material.Tf = {0xfc4340ff, RGBA::Type::eAbsolute};
				auto ent0 = spawn("model_1_0", m_data.groups["test_lit"], prim);
				ent0.get<SceneNode>().position({2.0f, -1.0f, 2.0f});
				m_data.entities["model_1_0"] = ent0;
			}
			if (m_store.contains<Model>("models/nanosuit")) {
				auto ent = spawn("model_1", "models/nanosuit", m_data.groups["test_lit"]);
				ent.get<SceneNode>().position({-1.0f, -2.0f, -3.0f});
				m_data.entities["model_1"] = ent;
			}
		}
	}

	void tick(Flags& out_flags, Time_s dt) {
		if constexpr (levk_editor) {
			edi::MenuList::Tree file;
			file.m_t.id = "File";
			file.push_front({"Quit", [&out_flags]() { out_flags.set(Flag::eClosed); }});
			Editor::s_in.menu.trees.push_back(std::move(file));
			Editor::s_in.registry = &m_data.registry;
			Editor::s_in.root = &m_data.root;
			Editor::s_in.customEntities.push_back(m_data.camera);
		}

		if (!m_data.loader.ready(&m_tasks)) { return; }
		if (m_data.registry.empty()) { init1(); }
		auto guiStack = m_data.registry.find<gui::ViewStack>(m_data.guiStack);
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
		auto& cam = m_data.registry.get<FreeCam>(m_data.camera);
		auto& pc = m_data.registry.get<PlayerController>(m_data.player);
		if (pc.active) {
			auto& node = m_data.registry.get<SceneNode>(m_data.player);
			m_data.registry.get<PlayerController>(m_data.player).tick(m_eng->inputState(), node, dt);
			glm::vec3 const& forward = node.orientation() * -graphics::front;
			cam.position = m_data.registry.get<SpringArm>(m_data.camera).tick(dt, node.position());
			cam.face(forward);
		} else {
			cam.tick(m_eng->inputState(), dt, m_eng->desktop());
		}
		m_data.registry.get<SceneNode>(m_data.entities["prop_1"]).rotate(glm::radians(360.0f) * dt.count(), graphics::up);
		if (auto node = m_data.registry.find<SceneNode>(m_data.entities["model_0_0"])) { node->rotate(glm::radians(-75.0f) * dt.count(), graphics::up); }
		if (auto node = m_data.registry.find<SceneNode>(m_data.entities["model_1_0"])) {
			static glm::quat s_axis = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
			s_axis = glm::rotate(s_axis, glm::radians(45.0f) * dt.count(), graphics::front);
			node->rotate(glm::radians(90.0f) * dt.count(), glm::normalize(s_axis * graphics::up));
		}
	}

	void render() {
		if (m_eng->drawReady()) {
			// write / update
			if (auto cam = m_data.registry.find<FreeCam>(m_data.camera)) {
				m_groups = SceneDrawer::groups(m_data.registry, true);
				m_drawDispatch.write(*cam, m_eng->framebufferSize(), m_data.dirLights, m_groups);
			}
			if (auto frame = m_eng->drawFrame(RGBA(0x777777ff, RGBA::Type::eAbsolute))) {
				if (!m_data.registry.empty()) {
					// draw
					frame->cmd().setViewportScissor(m_eng->viewport(), m_eng->scissor());
					SceneDrawer::draw(m_drawDispatch, m_groups, frame->cmd());
				}
				m_drawDispatch.swap();
			}
		}
	}

	std::vector<SceneDrawer::Group> m_groups;

  private:
	struct Data {
		std::unordered_map<Hash, decf::entity_t> entities;
		std::unordered_map<Hash, DrawGroup> groups;

		BitmapText text;
		std::vector<DirLight> dirLights;

		SceneNode::Root root;
		decf::registry_t registry;
		decf::entity_t camera;
		decf::entity_t player;
		decf::entity_t guiStack;
		AssetListLoader loader;
	};

	Data m_data;
	std::future<void> m_ready;

  public:
	AssetStore m_store;
	scheduler m_tasks;
	not_null<Engine*> m_eng;
	DrawDispatch m_drawDispatch;

	struct {
		input::Trigger editor = {input::Key::eE, input::Action::ePressed, input::Mod::eControl};
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
	try {
		window::Instance::CreateInfo winInfo;
		winInfo.config.androidApp = androidApp;
		winInfo.config.title = "levk demo";
		winInfo.config.size = {1280, 720};
		winInfo.options.bCentreCursor = true;
		window::Instance winst(winInfo);
		graphics::Bootstrap::CreateInfo bootInfo;
		bootInfo.instance.extensions = winst.vkInstanceExtensions();
		bootInfo.instance.bValidation = levk_debug;
		bootInfo.instance.validationLog = dl::level::info;
		std::optional<App> app;
		Engine engine(&winst, {});
		Flags flags;
		FlagsInput flagsInput(flags);
		engine.pushReceiver(&flagsInput);
		time::Point t = time::now();
		while (true) {
			Time_s dt = time::now() - t;
			t = time::now();
			auto [_, queue] = engine.poll(true);
			poll(flags, std::move(queue));
			if (flags.test(Flag::eClosed)) {
				app.reset();
				engine.unboot();
				break;
			}
			if (flags.test(Flag::ePaused)) { continue; }
			if (flags.test(Flag::eInit)) {
				app.reset();
				engine.boot(bootInfo);
				app.emplace(&engine, reader);
			}
			if (flags.test(Flag::eTerm)) {
				app.reset();
				engine.unboot();
			}

			if (engine.beginFrame(false)) {
				if (app) {
					// kt::kthread::sleep_for(5ms);
					app->tick(flags, dt);
					app->render();
				}
				flags.reset(Flags(Flag::eRecreated) | Flag::eInit | Flag::eTerm);
			}
		}
	} catch (std::exception const& e) { logE("exception: {}", e.what()); }
	return true;
}
} // namespace le::demo
