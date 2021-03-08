#include <iostream>
#include <core/transform.hpp>
#include <demo.hpp>
#include <dumb_json/djson.hpp>
#include <graphics/bitmap_text.hpp>
#include <graphics/common.hpp>
#include <graphics/context/bootstrap.hpp>
#include <graphics/geometry.hpp>
#include <graphics/mesh.hpp>
#include <graphics/render_context.hpp>
#include <graphics/shader.hpp>
#include <graphics/shader_buffer.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/utils.hpp>
#include <window/bootstrap.hpp>

#include <core/utils/algo.hpp>
#include <core/utils/std_hash.hpp>
#include <core/utils/string.hpp>
#include <dtasks/error_handler.hpp>
#include <dtasks/task_scheduler.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/camera.hpp>
#include <engine/input/input.hpp>
#include <engine/render/drawer.hpp>
#include <engine/render/model.hpp>

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

struct Font {
	io::Path atlasID;
	io::Path samplerID;
	io::Path materialID;
	std::optional<graphics::Texture> atlas;
	std::array<graphics::Glyph, maths::max<u8>()> glyphs;
	s32 orgSizePt = 0;

	graphics::Glyph deserialise(u8 c, dj::node_t const& json) {
		graphics::Glyph ret;
		ret.ch = c;
		ret.st = {json.get("x").as<s32>(), json.get("y").as<s32>()};
		ret.uv = ret.cell = {json.get("width").as<s32>(), json.get("height").as<s32>()};
		ret.offset = {json.get("originX").as<s32>(), json.get("originY").as<s32>()};
		auto const pAdvance = json.find("advance");
		ret.xAdv = pAdvance ? pAdvance->as<s32>() : ret.cell.x;
		if (auto pBlank = json.find("isBlank")) {
			ret.bBlank = pBlank->as<bool>();
		}
		return ret;
	}

	void deserialise(dj::node_t const& json) {
		if (auto pAtlas = json.find("sheetID")) {
			atlasID = pAtlas->as<std::string>();
		}
		if (auto pSampler = json.find("samplerID")) {
			samplerID = pSampler->as<std::string>();
		}
		if (auto pMaterial = json.find("materialID")) {
			materialID = pMaterial->as<std::string>();
		}
		if (auto pSize = json.find("size")) {
			orgSizePt = pSize->as<s32>();
		}
		if (auto pGlyphsData = json.find("glyphs")) {
			for (auto& [key, value] : pGlyphsData->as<dj::map_nodes_t>()) {
				if (!key.empty()) {
					graphics::Glyph const glyph = deserialise((u8)key[0], *value);
					if (glyph.cell.x > 0 && glyph.cell.y > 0) {
						glyphs[(std::size_t)glyph.ch] = glyph;
					} else {
						logW("Could not deserialise Glyph '{}'!", key[0]);
					}
				}
			}
		}
	}

	bool create(graphics::VRAM& vram, io::Reader const& reader, io::Path const& id, io::Path const& path, vk::Sampler sampler, vk::Format format) {
		auto jsonText = reader.string(path);
		if (!jsonText) {
			return false;
		}
		auto json = dj::node_t::make(*jsonText);
		if (!json) {
			return false;
		}
		deserialise(*json);
		auto bytes = reader.bytes(path.parent_path() / atlasID);
		if (!bytes) {
			return false;
		}
		atlas = graphics::Texture((id / "atlas").generic_string(), vram);
		graphics::Texture::CreateInfo info;
		info.sampler = sampler;
		info.data = graphics::Texture::Img{std::move(*bytes)};
		info.format = format;
		if (!atlas->construct(info)) {
			return false;
		}
		return true;
	}
};

struct Text {
	using Type = graphics::Mesh::Type;

	graphics::BitmapText text;
	std::optional<graphics::Mesh> mesh;
	static constexpr glm::mat4 const model = glm::mat4(1.0f);

	void create(graphics::VRAM& vram, io::Path const& id, Type type = Type::eDynamic) {
		mesh = graphics::Mesh((id / "mesh").generic_string(), vram, type);
	}

	bool set(Font const& font, std::string_view str) {
		text.text = str;
		if (mesh) {
			return mesh->construct(text.generate(font.glyphs, font.atlas->data().size));
		}
		return false;
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

class Engine {
  public:
	Engine(window::IInstance& inst) : m_win(inst), m_pDesktop(dynamic_cast<window::DesktopInstance*>(&inst)) {
	}

	Input::Out poll(bool consume) noexcept {
		auto ret = m_input.update(m_win.get().pollEvents(), consume, m_pDesktop);
		// TODO: pass state to editor etc
		for (Input::IContext& context : m_contexts) {
			if (context.block(ret.state)) {
				break;
			}
		}
		return ret;
	}

	void pushContext(Input::IContext& context) {
		context.m_inputToken = m_contexts.push<true>(context);
	}

	bool boot(graphics::Bootstrap::CreateInfo const& boot) {
		if (!m_boot && !m_gfx) {
			auto surface = [this](vk::Instance inst) { return makeSurface(inst); };
			m_boot.emplace(boot, surface, m_win.get().framebufferSize());
			m_gfx.emplace(m_boot->swapchain);
			return true;
		}
		return false;
	}

	bool unboot() noexcept {
		if (m_boot && m_gfx) {
			m_gfx.reset();
			m_boot.reset();
			return true;
		}
		return false;
	}

	bool booted() const noexcept {
		return m_boot.has_value() && m_gfx.has_value();
	}

	graphics::Bootstrap& boot() {
		ENSURE(m_boot.has_value(), "Not booted");
		return *m_boot;
	}

	graphics::RenderContext& context() {
		ENSURE(m_gfx.has_value(), "Not booted");
		return *m_gfx;
	}

	Ref<window::IInstance> m_win;

  private:
	vk::SurfaceKHR makeSurface(vk::Instance vkinst) {
		vk::SurfaceKHR ret;
		m_win.get().vkCreateSurface(vkinst, ret);
		return ret;
	}

	std::optional<graphics::Bootstrap> m_boot;
	std::optional<graphics::RenderContext> m_gfx;
	Input m_input;
	TTokenGen<Ref<Input::IContext>, TGSpec_deque> m_contexts;
	window::DesktopInstance* m_pDesktop = {};
};

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

struct DrawObj {
	graphics::ShaderBuffer matBuf;
	Primitive primitive;
};

struct Drawable {
	Transform tr;
	graphics::ShaderBuffer mat_m;
	std::vector<DrawObj> objs;
};

class Drawer {
  public:
	using type = Drawable;

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
				d.mat_m.write(d.tr.model());
				si.update(d.mat_m, sb10.set, sb10.bind, idx);
				d.mat_m.swap();
			}
			for (DrawObj& obj : d.objs) {
				Material const& mat = obj.primitive.material;
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
					ShadeMat const mat = ShadeMat::make(obj.primitive.material);
					obj.matBuf.write(mat);
					si.update(obj.matBuf, sb30.set, sb30.bind, idx);
					obj.matBuf.swap();
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
			for (DrawObj const& obj : d.objs) {
				pipe.bindSet(cb, {2, 3}, idx);
				ENSURE(obj.primitive.mesh, "Null mesh");
				obj.primitive.mesh->draw(cb);
				++idx;
			}
		}
	}
};

class App : public Input::IContext {
  public:
	App(Engine& eng, io::Reader const& reader) : m_eng(eng) {
		dts::g_error_handler = &g_taskErr;
		auto loadShader = [this](std::string_view id, io::Path v, io::Path f) {
			AssetLoadData<graphics::Shader> shaderLD{m_eng.get().boot().device, {}};
			shaderLD.shaderPaths[graphics::Shader::Type::eVertex] = std::move(v);
			shaderLD.shaderPaths[graphics::Shader::Type::eFragment] = std::move(f);
			m_store.load<graphics::Shader>(id, std::move(shaderLD));
		};
		using PCI = graphics::Pipeline::CreateInfo;
		auto loadPipe = [this](std::string_view id, Hash shaderID, graphics::PFlags flags = {}, std::optional<PCI> pci = std::nullopt) {
			AssetLoadData<graphics::Pipeline> pipelineLD{m_eng.get().context()};
			pipelineLD.name = id;
			pipelineLD.shaderID = shaderID;
			pipelineLD.info = pci;
			pipelineLD.flags = flags;
			m_store.load<graphics::Pipeline>(id, std::move(pipelineLD));
		};
		m_store.resources().reader(reader);
		m_store.add("samplers/default", graphics::Sampler{eng.boot().device, graphics::Sampler::info({vk::Filter::eLinear, vk::Filter::eLinear})});
		auto sampler = m_store.get<graphics::Sampler>("samplers/default");
		{
			task_scheduler::stage_t models;
			AssetLoadData<Model> ald(m_eng.get().boot().vram, *sampler);
			ald.modelID = "models/plant";
			ald.jsonID = "models/plant/plant.json";
			ald.texFormat = m_eng.get().context().textureFormat();
			models.tasks.push_back([ald, this]() { m_store.load<Model>(ald.modelID, ald); });

			ald.jsonID = "models/teapot/teapot.json";
			ald.modelID = "models/teapot";
			models.tasks.push_back([ald, this]() { m_store.load<Model>(ald.modelID, ald); });

			ald.jsonID = "models/test/nanosuit/nanosuit.json";
			if (m_store.resources().reader().present(ald.jsonID)) {
				ald.modelID = "models/nanosuit";
				models.tasks.push_back([ald, this]() { m_store.load<Model>(ald.modelID, ald); });
			}
			m_data.load_models = m_tasks.stage(std::move(models));
		}

		m_data.font.create(eng.boot().vram, reader, "fonts/default", "fonts/default.json", sampler->sampler(), eng.context().textureFormat());
		m_data.text.create(eng.boot().vram, "text");
		m_data.text.text.size = 80U;
		m_data.text.text.colour = colours::yellow;
		m_data.text.text.pos = {0.0f, 200.0f, 0.0f};
		m_data.text.set(m_data.font, "Hi!");
		{
			graphics::Geometry gcube = graphics::makeCube(0.5f);
			auto const skyCubeI = gcube.indices;
			auto const skyCubeV = gcube.positions();
			auto cube = m_store.add<graphics::Mesh>("meshes/cube", graphics::Mesh("meshes/cube", eng.boot().vram));
			cube->construct(gcube);
			m_data.mesh.push_back(cube.m_id);
			auto cone = m_store.add<graphics::Mesh>("meshes/cone", graphics::Mesh("meshes/cone", eng.boot().vram));
			cone->construct(graphics::makeCone());
			m_data.mesh.push_back(cone.m_id);
			auto skycube = m_store.add<graphics::Mesh>("skycube", graphics::Mesh("skycube", eng.boot().vram));
			skycube->construct(View<glm::vec3>(skyCubeV), skyCubeI);
			m_data.mesh.push_back(skycube.m_id);
		}

		task_scheduler::stage_id load_shaders;
		PCI pci_skybox = eng.context().pipeInfo();
		pci_skybox.fixedState.depthStencilState.depthWriteEnable = false;
		pci_skybox.fixedState.vertexInput = eng.context().vertexInput({0, sizeof(glm::vec3), {{vk::Format::eR32G32B32Sfloat, 0}}});
		{
			task_scheduler::stage_t shaders;
			shaders.tasks.push_back([loadShader]() { loadShader("shaders/test", "shaders/test.vert", "shaders/test.frag"); });
			shaders.tasks.push_back([loadShader]() { loadShader("shaders/test_tex", "shaders/test.vert", "shaders/test_tex.frag"); });
			shaders.tasks.push_back([loadShader]() { loadShader("shaders/test_lit", "shaders/test_lit.vert", "shaders/test_lit.frag"); });
			shaders.tasks.push_back([loadShader]() { loadShader("shaders/ui", "shaders/ui.vert", "shaders/ui.frag"); });
			shaders.tasks.push_back([loadShader]() { loadShader("shaders/skybox", "shaders/skybox.vert", "shaders/skybox.frag"); });
			load_shaders = m_tasks.stage(std::move(shaders));
		}
		{
			task_scheduler::stage_t pipes;
			pipes.tasks.push_back([loadPipe]() { loadPipe("pipelines/test", "shaders/test", graphics::PFlags::inverse()); });
			pipes.tasks.push_back([loadPipe]() { loadPipe("pipelines/test_tex", "shaders/test_tex", graphics::PFlags::inverse()); });
			pipes.tasks.push_back([loadPipe]() { loadPipe("pipelines/test_lit", "shaders/test_lit", graphics::PFlags::inverse()); });
			pipes.tasks.push_back([loadPipe]() { loadPipe("pipelines/ui", "shaders/ui", graphics::PFlags::inverse()); });
			pipes.tasks.push_back([loadPipe, pci_skybox]() { loadPipe("pipelines/skybox", "shaders/skybox", {}, pci_skybox); });
			pipes.deps.push_back(load_shaders);
			m_data.load_pipes = m_tasks.stage(std::move(pipes));
		}

		task_scheduler::stage_t texload;
		AssetLoadData<graphics::Texture> textureLD{eng.boot().vram};
		textureLD.samplerID = "samplers/default";
		textureLD.name = "cubemaps/sky_dusk";
		textureLD.prefix = "skyboxes/sky_dusk";
		textureLD.ext = ".jpg";
		textureLD.imageIDs = {"right", "left", "up", "down", "front", "back"};
		texload.tasks.push_back([this, textureLD]() {
			m_store.load<graphics::Texture>(textureLD.name, textureLD);
			m_data.tex.push_back(textureLD.name);
		});
		textureLD.name = "textures/container2/diffuse";
		textureLD.prefix.clear();
		textureLD.ext.clear();
		textureLD.imageIDs = {"textures/container2.png"};
		texload.tasks.push_back([this, textureLD]() {
			m_store.load<graphics::Texture>(textureLD.name, textureLD);
			m_data.tex.push_back(textureLD.name);
		});
		textureLD.name = "textures/container2/specular";
		textureLD.prefix.clear();
		textureLD.ext.clear();
		textureLD.imageIDs = {"textures/container2_specular.png"};
		texload.tasks.push_back([this, textureLD]() {
			m_store.load<graphics::Texture>(textureLD.name, textureLD);
			m_data.tex.push_back(textureLD.name);
		});
		textureLD.name = "textures/red";
		textureLD.imageIDs.clear();
		textureLD.raw.bytes = graphics::utils::convert({0xff, 0, 0, 0xff});
		textureLD.raw.size = {1, 1};
		texload.tasks.push_back([this, textureLD]() {
			m_store.load<graphics::Texture>(textureLD.name, textureLD);
			m_data.tex.push_back(textureLD.name);
		});
		textureLD.name = "textures/black";
		textureLD.raw.bytes = graphics::utils::convert({0, 0, 0, 0xff});
		texload.tasks.push_back([this, textureLD]() {
			m_store.load<graphics::Texture>(textureLD.name, textureLD);
			m_data.tex.push_back(textureLD.name);
		});
		textureLD.name = "textures/white";
		textureLD.raw.bytes = graphics::utils::convert({0xff, 0xff, 0xff, 0xff});
		texload.tasks.push_back([this, textureLD]() {
			m_store.load<graphics::Texture>(textureLD.name, textureLD);
			m_data.tex.push_back(textureLD.name);
		});
		textureLD.name = "textures/blank";
		textureLD.raw.bytes = graphics::utils::convert({0, 0, 0, 0});
		texload.tasks.push_back([this, textureLD]() {
			m_store.load<graphics::Texture>(textureLD.name, textureLD);
			m_data.tex.push_back(textureLD.name);
		});
		m_data.load_tex = m_tasks.stage(std::move(texload));
		m_eng.get().pushContext(*this);
		eng.m_win.get().show();
	}

	bool block(Input::State const& state) override {
		if (state.focus == Input::Focus::eGained) {
			m_store.update();
		}
		return false;
	}

	decf::spawn_t<Drawable, DrawLayer> spawn(std::string name, View<Primitive> primitives, DrawLayer const& layer) {
		auto ret = m_data.registry.spawn<Drawable, DrawLayer>(name);
		auto& [e, c] = ret;
		auto& [d, l] = c;
		l = layer;
		d.mat_m = graphics::ShaderBuffer(m_eng.get().boot().vram, name + "/mat_m", {});
		for (auto prim : primitives) {
			DrawObj obj;
			obj.primitive = prim;
			obj.matBuf = graphics::ShaderBuffer(m_eng.get().boot().vram, name + "/material", {});
			d.objs.push_back(std::move(obj));
		}
		return ret;
	};

	void init1() {
		auto pipe_test = m_store.find<graphics::Pipeline>("pipelines/test");
		auto pipe_testTex = m_store.find<graphics::Pipeline>("pipelines/test_tex");
		auto pipe_testLit = m_store.find<graphics::Pipeline>("pipelines/test_lit");
		auto pipe_ui = m_store.find<graphics::Pipeline>("pipelines/ui");
		auto pipe_sky = m_store.find<graphics::Pipeline>("pipelines/skybox");
		auto skycube = m_store.get<graphics::Mesh>("skycube");
		auto cube = m_store.get<graphics::Mesh>("meshes/cube");
		auto cone = m_store.get<graphics::Mesh>("meshes/cone");
		auto skymap = m_store.get<graphics::Texture>("cubemaps/sky_dusk");
		m_data.drawer.m_defaults.black = &m_store.get<graphics::Texture>("textures/black").get();
		m_data.drawer.m_defaults.white = &m_store.get<graphics::Texture>("textures/white").get();

		m_data.cam.position = {0.0f, 2.0f, 4.0f};
		m_data.layers["sky"] = DrawLayer{&pipe_sky->get(), -10};
		m_data.layers["test"] = DrawLayer{&pipe_test->get(), 0};
		m_data.layers["test_tex"] = DrawLayer{&pipe_testTex->get(), 0};
		m_data.layers["test_lit"] = DrawLayer{&pipe_testLit->get(), 0};
		m_data.layers["ui"] = DrawLayer{&pipe_ui->get(), 10};
		m_data.drawer.m_view.mats = graphics::ShaderBuffer(m_eng.get().boot().vram, "view_mats", {});
		{
			graphics::ShaderBuffer::CreateInfo info;
			info.type = vk::DescriptorType::eStorageBuffer;
			m_data.drawer.m_view.lights = graphics::ShaderBuffer(m_eng.get().boot().vram, "lights", {});
		}
		DirLight l0, l1;
		l0.direction = {-graphics::front, 0.0f};
		l1.direction = {-graphics::up, 0.0f};
		l0.albedo = Albedo::make(colours::cyan, {0.2f, 0.5f, 0.3f, 0.0f});
		l1.albedo = Albedo::make(colours::white, {0.4f, 1.0f, 0.8f, 0.0f});
		m_data.dirLights = {l0, l1};
		{
			Primitive prim;
			prim.material.map_Kd = &*skymap;
			prim.mesh = &*skycube;
			spawn("skybox", prim, m_data.layers["sky"]);
		}
		{
			Primitive prim;
			prim.mesh = &*cube;
			prim.material.map_Kd = &*m_store.get<graphics::Texture>("textures/container2/diffuse");
			prim.material.map_Ks = &*m_store.get<graphics::Texture>("textures/container2/specular");
			// d.mat.albedo.diffuse = colours::cyan.toVec3();
			m_data.entities["cube_tex"] = spawn("prop_0", prim, m_data.layers["test_lit"]);
		}
		{
			Primitive prim;
			prim.mesh = &*cube;
			auto ent = spawn("prop_1", prim, m_data.layers["test"]);
			ent.get<Drawable>().tr.position({-5.0f, -1.0f, -2.0f});
			m_data.entities["prop_1"] = ent;
		}
		{
			Primitive prim;
			prim.mesh = &*cone;
			auto ent = spawn("prop_2", prim, m_data.layers["test_tex"]);
			ent.get<Drawable>().tr.position({1.0f, -2.0f, -3.0f});
		}
		{
			Primitive prim;
			prim.material.map_Kd = &*m_data.font.atlas;
			prim.material.map_d = &*m_data.font.atlas;
			prim.mesh = &*m_data.text.mesh;
			spawn("ui_1", prim, m_data.layers["ui"]);
		}
		{
			if (auto model = m_store.find<Model>("models/plant")) {
				auto ent0 = spawn("model_0_0", model->get().primitives(), m_data.layers["test_lit"]);
				ent0.get<Drawable>().tr.position({-2.0f, -1.0f, 2.0f});
				m_data.entities["model_0_0"] = ent0;

				auto ent1 = spawn("model_0_1", model->get().primitives(), m_data.layers["test_lit"]);
				ent1.get<Drawable>().tr.position({-2.0f, -1.0f, 5.0f});
				m_data.entities["model_0_1"] = ent1;
			}
			if (auto model = m_store.find<Model>("models/teapot")) {
				Primitive prim = model->get().primitives().front();
				prim.material.Tf = Colour(0xfc2320ff);
				auto ent0 = spawn("model_1_0", prim, m_data.layers["test_lit"]);
				ent0.get<Drawable>().tr.position({2.0f, -1.0f, 2.0f});
				m_data.entities["model_1_0"] = ent0;
			}
			if (auto model = m_store.find<Model>("models/nanosuit")) {
				auto ent = spawn("model_1", model->get().primitives(), m_data.layers["test_lit"]);
				ent.get<Drawable>().tr.position({-1.0f, -2.0f, -3.0f});
				m_data.entities["model_1"] = ent;
			}
		}
	}

	bool ready(std::initializer_list<Ref<dts::task_scheduler::stage_id>> sts) const {
		for (dts::task_scheduler::stage_id& st : sts) {
			if (st.id > 0) {
				if (m_tasks.stage_done(st)) {
					st = {};
				} else {
					return false;
				}
			}
		}
		return true;
	}

	void tick(Time_s dt) {
		if (!ready({m_data.load_pipes, m_data.load_tex, m_data.load_models})) {
			return;
		}
		if (m_data.registry.empty()) {
			init1();
		}
		// camera
		{
			glm::vec3 const moveDir = glm::normalize(glm::cross(m_data.cam.position, graphics::up));
			m_data.cam.position += moveDir * dt.count() * 0.75f;
			m_data.cam.look(-m_data.cam.position);
		}
		m_data.registry.get<Drawable>(m_data.entities["cube_tex"]).tr.rotate(glm::radians(-180.0f) * dt.count(), glm::normalize(glm::vec3(1.0f)));
		m_data.registry.get<Drawable>(m_data.entities["prop_1"]).tr.rotate(glm::radians(360.0f) * dt.count(), graphics::up);
		if (auto d = m_data.registry.find<Drawable>(m_data.entities["model_0_0"])) {
			d->tr.rotate(glm::radians(-75.0f) * dt.count(), graphics::up);
		}
		if (auto d = m_data.registry.find<Drawable>(m_data.entities["model_1_0"])) {
			static glm::quat s_axis = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
			s_axis = glm::rotate(s_axis, glm::radians(45.0f) * dt.count(), graphics::front);
			d->tr.rotate(glm::radians(90.0f) * dt.count(), glm::normalize(s_axis * graphics::up));
		}
	}

	void render() {
		if (m_eng.get().context().waitForFrame()) {
			// write / update
			if (!m_data.registry.empty()) {
				m_data.drawer.write(m_data.cam, m_eng.get().context().extent(), m_data.dirLights);
			}

			// draw
			if (auto r = m_eng.get().context().render(Colour(0x040404ff))) {
				if (!m_data.registry.empty()) {
					auto& cb = r->primary();
					cb.setViewportScissor(m_eng.get().context().viewport(), m_eng.get().context().scissor());
					batchDraw(m_data.drawer, m_data.registry, cb);
				}
			}
		}
	}

  private:
	struct Data {
		std::vector<Hash> tex;
		std::vector<Hash> mesh;
		std::unordered_map<Hash, decf::entity_t> entities;
		std::unordered_map<Hash, DrawLayer> layers;
		Drawer drawer;

		Font font;
		Text text;
		Camera cam;
		std::vector<DirLight> dirLights;

		task_scheduler::stage_id load_pipes, load_tex, load_models;
		decf::registry_t registry;
	};

	Data m_data;
	task_scheduler m_tasks;
	std::future<void> m_ready;

  public:
	AssetStore m_store;
	Ref<Engine> m_eng;
};

struct FlagsInput : Input::IContext {
	Flags& flags;

	FlagsInput(Flags& flags) : flags(flags) {
	}

	bool block(Input::State const& state) override {
		bool ret = false;
		if (state.any({window::Key::eLeftControl, window::Key::eRightControl})) {
			if (state.pressed(window::Key::eW)) {
				flags.set(Flag::eClosed);
				ret = true;
			}
			if (state.released(window::Key::eD)) {
				flags.set(Flag::eDebug0);
				ret = true;
			}
		}
		return ret;
	}
};

bool run(CreateInfo const& info, io::Reader const& reader) {
	os::args({info.args.argc, info.args.argv});
	if (os::halt(g_cmdArgs)) {
		return true;
	}
	try {
		window::Instance::CreateInfo winInfo;
		winInfo.config.androidApp = info.androidApp;
		winInfo.config.title = "levk demo";
		winInfo.config.size = {1280, 720};
		winInfo.options.bCentreCursor = true;
		winInfo.options.verbosity = LibLogger::Verbosity::eLibrary;
		window::Instance winst(winInfo);
		graphics::Bootstrap::CreateInfo bootInfo;
		bootInfo.instance.extensions = winst.vkInstanceExtensions();
		bootInfo.instance.bValidation = levk_debug;
		bootInfo.instance.validationLog = dl::level::info;
		bootInfo.logVerbosity = LibLogger::Verbosity::eLibrary;
		std::optional<App> app;
		bootInfo.device.pickOverride = GPUPicker::s_picked;
		Engine engine(winst);
		Flags flags;
		FlagsInput flagsInput(flags);
		engine.pushContext(flagsInput);
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
			if (engine.booted() && engine.context().reconstructed(winst.framebufferSize())) {
				continue;
			}

			if (app) {
				// threads::sleep(5ms);
				app->tick(dt);
				app->render();
			}
			flags.reset(Flag::eRecreated | Flag::eInit | Flag::eTerm);
		}
	} catch (std::exception const& e) {
		logE("exception: {}", e.what());
	}
	return true;
}
} // namespace le::demo
