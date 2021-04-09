#include <iostream>
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
#include <engine/render/drawer.hpp>
#include <engine/render/model.hpp>
#include <engine/scene_node.hpp>
#include <graphics/common.hpp>
#include <graphics/shader_buffer.hpp>
#include <graphics/utils/utils.hpp>
#include <window/bootstrap.hpp>

// #include <engine/gui/node.hpp>

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
			if (resize.bFramebuffer) {
				out_flags.set(Flag::eResized);
			}
			break;
		}
		case window::Event::Type::eInit:
			out_flags.set(Flag::eInit);
			break;
		case window::Event::Type::eTerm:
			out_flags.set(Flag::eTerm);
			break;
		default:
			break;
		}
	}
}

struct GPULister : os::ICmdArg {
	inline static constexpr std::array names = {"gpu-list"sv, "list-gpus"sv};

	View<std::string_view> keyVariants() const override {
		return names;
	}

	bool halt(std::string_view) override {
		graphics::g_log.minVerbosity = LibLogger::Verbosity::eEndUser;
		graphics::Instance inst(graphics::Instance::CreateInfo{});
		std::stringstream str;
		str << "Available GPUs:\n";
		int i = 0;
		for (auto const& d : inst.availableDevices(graphics::Device::requiredExtensions)) {
			str << '\t' << i++ << ". " << d << "\n";
		}
		str << "\n";
		std::cout << str.str();
		return true;
	}

	Usage usage() const override {
		return {"", "List supported GPUs"};
	}
};

struct GPUPicker : os::ICmdArg {
	inline static constexpr std::array names = {"use-gpu"sv, "pick-gpu"sv};

	inline static std::optional<std::size_t> s_picked;

	View<std::string_view> keyVariants() const override {
		return names;
	}

	bool halt(std::string_view params) override {
		s32 idx = utils::to<s32>(params, -1);
		if (idx >= 0) {
			s_picked = (std::size_t)idx;
			logD("Using custom GPU index: {}", idx);
		}
		return false;
	}

	Usage usage() const override {
		return {"<0-...>", "Select a custom available GPU"};
	}
};

void listCmdArgs();

struct HelpCmd : os::ICmdArg {
	inline static constexpr std::array names = {"h"sv, "help"sv};

	View<std::string_view> keyVariants() const override {
		return names;
	}

	bool halt(std::string_view) override {
		listCmdArgs();
		return true;
	}

	Usage usage() const override {
		return {"", "List all command line arguments"};
	}
};

struct Text {
	using Type = graphics::Mesh::Type;

	graphics::BitmapText text;
	std::optional<graphics::Mesh> mesh;
	static constexpr glm::mat4 const model = glm::mat4(1.0f);

	void create(graphics::VRAM& vram, Type type = Type::eDynamic) {
		mesh = graphics::Mesh(vram, type);
	}

	bool set(BitmapFont const& font, std::string_view str) {
		text.text = str;
		if (mesh) {
			return mesh->construct(text.generate(font.glyphs(), font.atlas().data().size));
		}
		return false;
	}

	Primitive primitive(BitmapFont const& font) const {
		Primitive ret;
		if (mesh) {
			ret.material.map_Kd = &font.atlas();
			ret.material.map_d = &font.atlas();
			ret.mesh = &*mesh;
		}
		return ret;
	}
};

GPULister g_gpuLister;
GPUPicker g_gpuPicker;
HelpCmd g_help;
std::array<Ref<os::ICmdArg>, 3> const g_cmdArgs = {g_gpuLister, g_gpuPicker, g_help};

void listCmdArgs() {
	std::stringstream str;
	for (os::ICmdArg const& arg : g_cmdArgs) {
		str << '[';
		bool bFirst = true;
		for (auto key : arg.keyVariants()) {
			if (!bFirst) {
				str << ", ";
			}
			bFirst = false;
			str << (key.length() == 1 ? "-"sv : "--"sv) << key;
		}
		auto const u = arg.usage();
		if (!u.params.empty()) {
			str << '=' << u.params;
		}
		str << "] : " << u.summary << '\n';
	}
	std::cout << str.str();
}

using namespace dts;

struct TaskErr : error_handler_t {
	void operator()(std::runtime_error const& err, u64) const override {
		ENSURE(false, err.what());
	}
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
		Albedo ret;
		glm::vec3 const c = colour.toVec3();
		f32 const a = colour.a.toF32();
		ret.ambient = {c * amdispsh.r, a};
		ret.diffuse = {c * amdispsh.g, a};
		ret.specular = {c * amdispsh.b, amdispsh.a};
		return ret;
	}
};

struct ShadeMat {
	enum Flag { eDrop = 1 << 0 };

	alignas(16) Albedo albedo;
	alignas(16) glm::vec4 tint;

	static ShadeMat make(Colour tint = colours::white, Colour colour = colours::white, glm::vec4 const& amdispsh = {0.5f, 0.8f, 0.4f, 42.0f}) noexcept {
		ShadeMat ret;
		ret.albedo = Albedo::make(colour, amdispsh);
		ret.tint = tint.toVec4();
		return ret;
	}

	static ShadeMat make(Material const& mtl) noexcept {
		ShadeMat ret;
		ret.albedo.ambient = mtl.Ka.toVec4();
		ret.albedo.diffuse = mtl.Kd.toVec4();
		ret.albedo.specular = mtl.Ks.toVec4();
		ret.tint = {mtl.Tf.toVec3(), mtl.d};
		return ret;
	}
};

struct DirLight {
	alignas(16) Albedo albedo;
	alignas(16) glm::vec4 direction;
};

struct DirLights {
	alignas(16) std::array<DirLight, 4> lights;
	alignas(16) u32 count = 0;
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

	void tick(Input::State const& state, SceneNode& node, Time_s dt) noexcept {
		Control::Range r(Control::KeyRange{Input::Key::eA, Input::Key::eD});
		roll = r(state);
		if (maths::abs(roll) < 0.25f) {
			roll = 0.0f;
		} else if (maths::abs(roll) > 0.9f) {
			roll = roll < 0.0f ? -1.0f : 1.0f;
		}
		r = Control::KeyRange{Input::Key::eS, Input::Key::eW};
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

struct Drawable {
	using Mesh = graphics::Mesh;

	std::vector<Primitive> prims;
	Ref<SceneNode> node;

	Drawable(SceneNode& node, View<Primitive> primitives) : node(node) {
		set(primitives);
	}

	Drawable(SceneNode& node, Mesh const& mesh, Material const& mat) : node(node) {
		Primitive prim;
		prim.mesh = &mesh;
		prim.material = mat;
		set(prim);
	}

	Drawable(SceneNode& node, Model const& model) : node(node) {
		set(model.primitives());
	}

	void set(View<Primitive> primitives) {
		prims.reserve(std::max(prims.size(), primitives.size()));
		for (std::size_t i = 0; i < primitives.size(); ++i) {
			if (i < prims.size()) {
				prims[i] = primitives[i];
			} else {
				prims.push_back(primitives[i]);
			}
		}
	}
};

class Drawer {
  public:
	using type = Drawable;
	graphics::VRAM* m_vram = {};

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

		SetBind(graphics::ShaderInput const& si, u32 s, u32 b) : set(s), bind(b), valid(si.contains(s, b)) {
		}

		explicit operator bool() const noexcept {
			return valid;
		}
	};

	void write(Camera const& cam, glm::vec2 fb, View<DirLight> lights) {
		ViewMats const v{cam.view(), cam.perspective(fb), cam.ortho(fb), {cam.position, 1.0f}};
		m_view.mats.write(v);
		if (!lights.empty()) {
			DirLights dl;
			for (std::size_t idx = 0; idx < lights.size() && idx < dl.lights.size(); ++idx) {
				dl.lights[idx] = lights[idx];
			}
			dl.count = (u32)lights.size();
			m_view.lights.write(dl);
		}
	}

	void update(DrawList<type> const& list) const {
		auto& si = list.layer.pipeline->shaderInput();
		si.update(m_view.mats, 0, 0, 0);
		if (list.layer.order >= 0 && si.contains(0, 1)) {
			si.update(m_view.lights, 0, 1, 0);
		}
		auto const sb10 = SetBind(si, 1, 0);
		auto const sb20 = SetBind(si, 2, 0);
		auto const sb21 = SetBind(si, 2, 1);
		auto const sb22 = SetBind(si, 2, 2);
		auto const sb30 = SetBind(si, 3, 0);
		std::size_t idx = 0;
		for (type& d : list.ts) {
			if (sb10) {
				graphics::Buffer const buf = m_vram->createBO(d.node.get().model(), vk::BufferUsageFlagBits::eUniformBuffer);
				si.update(buf, sb10.set, sb10.bind, idx);
			}
			for (Primitive& prim : d.prims) {
				Material const& mat = prim.material;
				if (list.layer.order < 0) {
					ENSURE(mat.map_Kd, "Null cubemap");
					si.update(*mat.map_Kd, 0, 1, idx);
				}
				if (sb20) {
					si.update(mat.map_Kd ? *mat.map_Kd : *m_defaults.white, sb20.set, sb20.bind, idx);
				}
				if (sb21) {
					si.update(mat.map_d ? *mat.map_d : *m_defaults.white, sb21.set, sb21.bind, idx);
				}
				if (sb22) {
					si.update(mat.map_Ks ? *mat.map_Ks : *m_defaults.black, sb22.set, sb22.bind, idx);
				}
				if (sb30) {
					graphics::Buffer const buf = m_vram->createBO(ShadeMat::make(mat), vk::BufferUsageFlagBits::eUniformBuffer);
					si.update(buf, sb30.set, sb30.bind, idx);
				}
				++idx;
			}
		}
	}

	void swap() {
		m_view.mats.swap();
		m_view.lights.swap();
	}

	void draw(graphics::CommandBuffer const& cb, DrawList<type> const& list) const {
		graphics::Pipeline const& pipe = *list.layer.pipeline;
		pipe.bindSet(cb, 0, 0);
		std::size_t idx = 0;
		for (Drawable const& d : list.ts) {
			pipe.bindSet(cb, 1, idx);
			for (Primitive const& prim : d.prims) {
				pipe.bindSet(cb, {2, 3}, idx);
				ENSURE(prim.mesh, "Null mesh");
				prim.mesh->draw(cb);
				++idx;
			}
		}
	}
};

class App : public Input::IReceiver {
  public:
	App(Engine& eng, io::Reader const& reader) : m_eng(eng) {
		dts::g_error_handler = &g_taskErr;
		auto loadShader = [this](std::string_view id, io::Path v, io::Path f) {
			AssetLoadData<graphics::Shader> shaderLD{m_eng.get().gfx().boot.device};
			shaderLD.name = id;
			shaderLD.shaderPaths[graphics::Shader::Type::eVertex] = std::move(v);
			shaderLD.shaderPaths[graphics::Shader::Type::eFragment] = std::move(f);
			return shaderLD;
		};
		using PCI = graphics::Pipeline::CreateInfo;
		auto loadPipe = [this](std::string_view id, Hash shaderID, graphics::PFlags flags = {}, std::optional<PCI> pci = std::nullopt) {
			AssetLoadData<graphics::Pipeline> pipelineLD{m_eng.get().gfx().context};
			pipelineLD.name = id;
			pipelineLD.shaderID = shaderID;
			pipelineLD.info = pci;
			pipelineLD.flags = flags;
			return pipelineLD;
		};
		m_store.resources().reader(reader);
		m_store.add("samplers/default", graphics::Sampler{eng.gfx().boot.device, graphics::Sampler::info({vk::Filter::eLinear, vk::Filter::eLinear})});
		{
			AssetList<Model> models;
			AssetLoadData<Model> ald(m_eng.get().gfx().boot.vram);
			ald.modelID = "models/plant";
			ald.jsonID = "models/plant/plant.json";
			ald.samplerID = "samplers/default";
			ald.texFormat = m_eng.get().gfx().context.textureFormat();
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

		AssetLoadData<BitmapFont> fld(m_eng.get().gfx().boot.vram);
		fld.jsonID = "fonts/default/default.json";
		fld.texFormat = m_eng.get().gfx().context.textureFormat();
		fld.samplerID = "samplers/default";
		m_data.loader.stage(m_store, AssetList<BitmapFont>{{{"fonts/default", std::move(fld)}}}, m_tasks);
		{
			graphics::Geometry gcube = graphics::makeCube(0.5f);
			auto const skyCubeI = gcube.indices;
			auto const skyCubeV = gcube.positions();
			auto cube = m_store.add<graphics::Mesh>("meshes/cube", graphics::Mesh(eng.gfx().boot.vram));
			cube->construct(gcube);
			auto cone = m_store.add<graphics::Mesh>("meshes/cone", graphics::Mesh(eng.gfx().boot.vram));
			cone->construct(graphics::makeCone());
			auto skycube = m_store.add<graphics::Mesh>("skycube", graphics::Mesh(eng.gfx().boot.vram));
			skycube->construct(View<glm::vec3>(skyCubeV), skyCubeI);
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
			static PCI pci_skybox = eng.gfx().context.pipeInfo();
			pci_skybox.fixedState.depthStencilState.depthWriteEnable = false;
			pci_skybox.fixedState.vertexInput = eng.gfx().context.vertexInput({0, sizeof(glm::vec3), {{vk::Format::eR32G32B32Sfloat, 0}}});
			pipes.add("pipelines/basic", loadPipe("pipelines/basic", "shaders/basic", graphics::PFlags::inverse()));
			pipes.add("pipelines/tex", loadPipe("pipelines/tex", "shaders/tex", graphics::PFlags::inverse()));
			pipes.add("pipelines/lit", loadPipe("pipelines/lit", "shaders/lit", graphics::PFlags::inverse()));
			pipes.add("pipelines/ui", loadPipe("pipelines/ui", "shaders/ui", graphics::PFlags::inverse()));
			pipes.add("pipelines/skybox", loadPipe("pipelines/skybox", "shaders/skybox", {}, pci_skybox));
			m_data.loader.stage(m_store, pipes, m_tasks, load_shaders);
		}

		AssetList<graphics::Texture> texList;
		AssetLoadData<graphics::Texture> textureLD{eng.gfx().boot.vram};
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
		textureLD.raw.bytes = graphics::utils::convert({0xff, 0, 0, 0xff});
		textureLD.raw.size = {1, 1};
		texList.add("textures/red", std::move(textureLD));
		textureLD.raw.bytes = graphics::utils::convert({0, 0, 0, 0xff});
		texList.add("textures/black", std::move(textureLD));
		textureLD.raw.bytes = graphics::utils::convert({0xff, 0xff, 0xff, 0xff});
		texList.add("textures/white", std::move(textureLD));
		textureLD.raw.bytes = graphics::utils::convert({0, 0, 0, 0});
		texList.add("textures/blank", std::move(textureLD));
		m_data.loader.stage(m_store, texList, m_tasks);
		m_eng.get().pushReceiver(*this);
		eng.m_win.get().show();

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

	bool block(Input::State const& state) override {
		if (state.focus == Input::Focus::eGained) {
			m_store.update();
		}
		if (m_controls.editor(state)) {
			Editor::s_engaged = !Editor::s_engaged;
		}
		return false;
	}

	decf::spawn_t<SceneNode> spawn(std::string name, DrawLayer const& layer) {
		auto ret = m_data.registry.spawn<SceneNode>(name, m_data.root);
		ret.get<SceneNode>().entity(ret);
		m_data.registry.attach<DrawLayer>(ret, layer);
		return ret;
	}

	decf::spawn_t<SceneNode> spawn(std::string name, View<Primitive> primitives, DrawLayer const& layer) {
		auto ret = spawn(std::move(name), layer);
		m_data.registry.attach<Drawable>(ret, ret.get<SceneNode>(), primitives);
		return ret;
	};

	decf::spawn_t<SceneNode> spawn(std::string name, DrawLayer const& layer, graphics::Mesh const& mesh, Material const& mat = {}) {
		auto ret = spawn(std::move(name), layer);
		m_data.registry.attach<Drawable>(ret, ret.get<SceneNode>(), mesh, mat);
		return ret;
	};

	decf::spawn_t<SceneNode> spawn(std::string name, DrawLayer const& layer, Model const& model) {
		auto ret = spawn(std::move(name), layer);
		m_data.registry.attach<Drawable>(ret, ret.get<SceneNode>(), model);
		return ret;
	};

	void init1() {
		auto pipe_test = m_store.find<graphics::Pipeline>("pipelines/basic");
		auto pipe_testTex = m_store.find<graphics::Pipeline>("pipelines/tex");
		auto pipe_testLit = m_store.find<graphics::Pipeline>("pipelines/lit");
		auto pipe_ui = m_store.find<graphics::Pipeline>("pipelines/ui");
		auto pipe_sky = m_store.find<graphics::Pipeline>("pipelines/skybox");
		auto skycube = m_store.get<graphics::Mesh>("skycube");
		auto cube = m_store.get<graphics::Mesh>("meshes/cube");
		auto cone = m_store.get<graphics::Mesh>("meshes/cone");
		auto skymap = m_store.get<graphics::Texture>("cubemaps/sky_dusk");
		auto font = m_store.get<BitmapFont>("fonts/default");
		m_data.drawer.m_defaults.black = &m_store.get<graphics::Texture>("textures/black").get();
		m_data.drawer.m_defaults.white = &m_store.get<graphics::Texture>("textures/white").get();

		m_data.text.create(m_eng.get().gfx().boot.vram);
		m_data.text.text.size = 80U;
		m_data.text.text.colour = colours::yellow;
		m_data.text.text.pos = {0.0f, 200.0f, 0.0f};
		m_data.text.set(font.get(), "Hi!");

		auto freecam = m_data.registry.spawn<FreeCam, SpringArm>("freecam");
		m_data.camera = freecam;
		auto& cam = freecam.get<FreeCam>();
		cam.position = {0.0f, 0.5f, 4.0f};
		cam.look({});
		auto& spring = freecam.get<SpringArm>();
		spring.position = cam.position;
		spring.offset = spring.position;

		m_data.layers["sky"] = DrawLayer{&pipe_sky->get(), -10};
		m_data.layers["test"] = DrawLayer{&pipe_test->get(), 0};
		m_data.layers["test_tex"] = DrawLayer{&pipe_testTex->get(), 0};
		m_data.layers["test_lit"] = DrawLayer{&pipe_testLit->get(), 0};
		m_data.layers["ui"] = DrawLayer{&pipe_ui->get(), 10};
		m_data.drawer.m_view.mats = graphics::ShaderBuffer(m_eng.get().gfx().boot.vram, {});
		{
			graphics::ShaderBuffer::CreateInfo info;
			info.type = vk::DescriptorType::eStorageBuffer;
			m_data.drawer.m_view.lights = graphics::ShaderBuffer(m_eng.get().gfx().boot.vram, {});
		}
		DirLight l0, l1;
		l0.direction = {-graphics::front, 0.0f};
		l1.direction = {-graphics::up, 0.0f};
		l0.albedo = Albedo::make(colours::cyan, {0.2f, 0.5f, 0.3f, 0.0f});
		l1.albedo = Albedo::make(colours::white, {0.4f, 1.0f, 0.8f, 0.0f});
		m_data.dirLights = {l0, l1};
		{
			Material mat;
			mat.map_Kd = &*skymap;
			spawn("skybox", m_data.layers["sky"], *skycube, mat);
		}
		{
			Material mat;
			mat.map_Kd = &*m_store.get<graphics::Texture>("textures/container2/diffuse");
			mat.map_Ks = &*m_store.get<graphics::Texture>("textures/container2/specular");
			// d.mat.albedo.diffuse = colours::cyan.toVec3();
			m_data.player = spawn("player", m_data.layers["test_lit"], *cube, mat);
			m_data.registry.attach<PlayerController>(m_data.player);
		}
		{
			auto ent = spawn("prop_1", m_data.layers["test"], *cube);
			ent.get<SceneNode>().position({-5.0f, -1.0f, -2.0f});
			m_data.entities["prop_1"] = ent;
		}
		{
			auto ent = spawn("prop_2", m_data.layers["test_tex"], *cone);
			ent.get<SceneNode>().position({1.0f, -2.0f, -3.0f});
		}
		{ spawn("ui_1", m_data.text.primitive(*font), m_data.layers["ui"]); }
		{
			if (auto model = m_store.find<Model>("models/plant")) {
				auto ent0 = spawn("model_0_0", m_data.layers["test_lit"], **model);
				ent0.get<SceneNode>().position({-2.0f, -1.0f, 2.0f});
				m_data.entities["model_0_0"] = ent0;

				auto ent1 = spawn("model_0_1", m_data.layers["test_lit"], **model);
				auto& node = ent1.get<SceneNode>();
				node.position({-2.0f, -1.0f, 5.0f});
				m_data.entities["model_0_1"] = ent1;
				node.parent(m_data.registry.get<SceneNode>(m_data.entities["model_0_0"]));
			}
			if (auto model = m_store.find<Model>("models/teapot")) {
				Primitive prim = model->get().primitives().front();
				prim.material.Tf = Colour(0xfc2320ff);
				auto ent0 = spawn("model_1_0", prim, m_data.layers["test_lit"]);
				ent0.get<SceneNode>().position({2.0f, -1.0f, 2.0f});
				m_data.entities["model_1_0"] = ent0;
			}
			if (auto model = m_store.find<Model>("models/nanosuit")) {
				auto ent = spawn("model_1", m_data.layers["test_lit"], **model);
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
			m_eng.get().update();
		}

		if (!m_data.loader.ready(&m_tasks)) {
			return;
		}
		if (m_data.registry.empty()) {
			init1();
		}
		auto& cam = m_data.registry.get<FreeCam>(m_data.camera);
		auto& pc = m_data.registry.get<PlayerController>(m_data.player);
		if (pc.active) {
			auto& node = m_data.registry.get<SceneNode>(m_data.player);
			m_data.registry.get<PlayerController>(m_data.player).tick(m_eng.get().inputState(), node, dt);
			glm::vec3 const& forward = node.orientation() * -graphics::front;
			cam.position = m_data.registry.get<SpringArm>(m_data.camera).tick(dt, node.position());
			cam.face(forward);
		} else {
			cam.tick(m_eng.get().inputState(), dt, m_eng.get().desktop());
		}
		m_data.registry.get<SceneNode>(m_data.entities["prop_1"]).rotate(glm::radians(360.0f) * dt.count(), graphics::up);
		if (auto node = m_data.registry.find<SceneNode>(m_data.entities["model_0_0"])) {
			node->rotate(glm::radians(-75.0f) * dt.count(), graphics::up);
		}
		if (auto node = m_data.registry.find<SceneNode>(m_data.entities["model_1_0"])) {
			static glm::quat s_axis = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
			s_axis = glm::rotate(s_axis, glm::radians(45.0f) * dt.count(), graphics::front);
			node->rotate(glm::radians(90.0f) * dt.count(), glm::normalize(s_axis * graphics::up));
		}
	}

	void render() {
		Engine& eng = m_eng;
		if (eng.gfx().context.waitForFrame()) {
			// write / update
			if (!m_data.registry.empty()) {
				auto& cam = m_data.registry.get<FreeCam>(m_data.camera);
				m_data.drawer.write(cam, eng.gfx().context.extent(), m_data.dirLights);
			}

			// draw
			if (auto r = eng.gfx().context.render(Colour(0x040404ff))) {
				eng.gfx().imgui.render();
				auto& cb = r->primary();
				if (!m_data.registry.empty()) {
					m_data.drawer.m_vram = &m_eng.get().gfx().boot.vram;
					cb.setViewportScissor(eng.viewport(), eng.gfx().context.scissor());
					batchDraw(m_data.drawer, m_data.registry, cb);
				}
				eng.gfx().imgui.endFrame(cb);
			}
		}
	}

  private:
	struct Data {
		std::unordered_map<Hash, decf::entity_t> entities;
		std::unordered_map<Hash, DrawLayer> layers;
		Drawer drawer;

		Text text;
		std::vector<DirLight> dirLights;

		SceneNode::Root root;
		decf::registry_t registry;
		decf::entity_t camera;
		decf::entity_t player;
		AssetListLoader loader;
	};

	Data m_data;
	std::future<void> m_ready;

  public:
	AssetStore m_store;
	scheduler m_tasks;
	Ref<Engine> m_eng;

	struct {
		Control::Trigger editor = {Input::Key::eE, Input::Action::ePressed, Input::Mod::eControl};
	} m_controls;
};

struct FlagsInput : Input::IReceiver {
	Flags& flags;

	FlagsInput(Flags& flags) : flags(flags) {
	}

	bool block(Input::State const& state) override {
		bool ret = false;
		if (auto key = state.released(Input::Key::eW); key && key->mods[Input::Mod::eControl]) {
			flags.set(Flag::eClosed);
			ret = true;
		}
		if (auto key = state.pressed(Input::Key::eD); key && key->mods[Input::Mod::eControl]) {
			flags.set(Flag::eDebug0);
			ret = true;
		}
		return ret;
	}
};

bool run(io::Reader const& reader, ErasedRef androidApp) {
	if (os::halt(g_cmdArgs)) {
		return true;
	}
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
		bootInfo.device.pickOverride = GPUPicker::s_picked;
		Engine engine(winst, {});
		Flags flags;
		FlagsInput flagsInput(flags);
		engine.pushReceiver(flagsInput);
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
			if (flags.test(Flag::ePaused)) {
				continue;
			}
			if (flags.test(Flag::eInit)) {
				app.reset();
				engine.boot(bootInfo);
				app.emplace(engine, reader);
			}
			if (flags.test(Flag::eTerm)) {
				app.reset();
				engine.unboot();
			}
			if (flags.test(Flag::eResized)) {
				/*if (!context.recreated(winst.framebufferSize())) {
					ENSURE(false, "Swapchain failure");
				}*/
				flags.reset(Flag::eResized);
			}
			if (engine.booted() && engine.gfx().context.reconstructed(winst.framebufferSize())) {
				continue;
			}

			if (app) {
				// threads::sleep(5ms);
				app->tick(flags, dt);
				app->render();
			}
			flags.reset(Flags(Flag::eRecreated) | Flag::eInit | Flag::eTerm);
		}
	} catch (std::exception const& e) {
		logE("exception: {}", e.what());
	}
	return true;
}
} // namespace le::demo
