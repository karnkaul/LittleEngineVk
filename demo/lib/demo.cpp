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

#include <engine/assets/asset_store.hpp>

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
		case window::Event::Type::eInput: {
			auto const& input = e->payload.input;
			if (input.key == window::Key::eW && input.action == window::Action::eRelease && input.mods[window::Mod::eControl]) {
				out_flags.set(Flag::eClosed);
			}
			if (input.key == window::Key::eD && input.action == window::Action::eRelease && input.mods[window::Mod::eControl]) {
				out_flags.flip(Flag::eDebug0);
			}
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

	Span<std::string_view> keyVariants() const override {
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

	Span<std::string_view> keyVariants() const override {
		return names;
	}

	bool halt(std::string_view params) override {
		s32 idx = utils::strings::toS32(params, -1);
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

	Span<std::string_view> keyVariants() const override {
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

struct Sets {
	std::unordered_map<u32, graphics::SetFactory> sets;

	void make(Span<u32> setNumbers, graphics::Pipeline const& pipe) {
		for (u32 num : setNumbers) {
			sets.emplace(num, pipe.makeSetFactory(num));
		}
	}

	graphics::SetFactory& operator[](u32 set) {
		if (auto it = sets.find(set); it != sets.end()) {
			return it->second;
		}
		ENSURE(false, "Nonexistent set");
		throw std::runtime_error("Nonexistent set");
	}

	void swap() {
		for (auto& [_, set] : sets) {
			set.swap();
		}
	}
};

struct SetLayouts {
	std::unordered_map<Hash, Sets> sets;

	void make(Hash layout, Span<u32> setNumbers, graphics::Pipeline const& pipe) {
		sets[layout].make(setNumbers, pipe);
	}

	Sets& operator[](Hash hash) {
		if (auto it = sets.find(hash); it != sets.end()) {
			return it->second;
		}
		ENSURE(false, "Nonexistent layout");
		throw std::runtime_error("Nonexistent layout");
	}

	void swap() {
		for (auto& [_, s] : sets) {
			s.swap();
		}
	}
};

struct Material {
	virtual void write(graphics::DescriptorSet&) {
	}
	virtual void bind(graphics::CommandBuffer&, graphics::Pipeline const&, graphics::DescriptorSet const&) const {
	}
};

struct TexturedMaterial : Material {
	graphics::Texture const* diffuse = {};
	u32 binding = 0;

	void write(graphics::DescriptorSet& ds) override {
		ENSURE(diffuse, "Null pipeline/texture view");
		ds.updateTextures(binding, *diffuse);
	}
	void bind(graphics::CommandBuffer& cb, graphics::Pipeline const& pi, graphics::DescriptorSet const& ds) const override {
		ENSURE(diffuse, "Null texture view");
		cb.bindSets(pi.layout(), ds.get(), ds.setNumber());
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
	graphics::BitmapText text;
	std::optional<graphics::Mesh> mesh;
	glm::mat4 model = glm::mat4(1.0f);

	void create(graphics::VRAM& vram, io::Path const& id) {
		mesh = graphics::Mesh((id / "mesh").generic_string(), vram);
	}

	bool set(Font const& font, std::string_view str) {
		text.text = str;
		if (mesh) {
			return mesh->construct(text.generate(font.glyphs, font.atlas->data().size));
		}
		return false;
	}
};

struct VP {
	glm::mat4 mat_p;
	glm::mat4 mat_v;
	glm::mat4 mat_ui;
};

struct Skybox {
	graphics::Mesh const* mesh = {};
	graphics::Texture const* cubemap = {};

	bool ready() const {
		return mesh && mesh->ready() && cubemap && cubemap->ready();
	}
	void update(graphics::DescriptorSet& set, graphics::Buffer const& vp) const {
		if (ready()) {
			set.updateBuffers(0, Ref<graphics::Buffer const>(vp), sizeof(VP));
			set.updateTextures(1, *cubemap);
		}
	}
	void draw(graphics::CommandBuffer& cb, graphics::Pipeline const& pi, graphics::DescriptorSet const& set) {
		if (ready()) {
			cb.bindPipe(pi);
			cb.bindSets(pi.layout(), set.get(), set.setNumber());
			cb.bindVBO(mesh->vbo().buffer, &mesh->ibo().buffer.get());
			cb.drawIndexed(mesh->ibo().count);
		}
	}
};

struct Prop2 {
	graphics::ShaderBuffer<glm::mat4, false> m;
	Transform transform;
	graphics::Mesh* mesh = {};
	Material* material = {};
};

struct Scene {
	using PropMap = std::unordered_map<Ref<graphics::Pipeline>, std::vector<Prop2>>;
	struct SetRefs {
		graphics::SetFactory& vp;
		graphics::SetFactory& m;
		graphics::SetFactory& mat;
		graphics::SetFactory& sky;
	};
	struct PipeRefs {
		graphics::Pipeline& main;
		graphics::Pipeline& sky;
	};

	std::optional<graphics::ShaderBuffer<VP, false>> vp;
	Skybox skybox;
	PropMap props;
	PropMap ui;

	void update(SetRefs sets) {
		vp->write();
		vp->update(sets.vp.front(), 0);
		vp->update(sets.sky.front(), 0);
		vp->swap();
		sets.sky.front().updateTextures(1, *skybox.cubemap);
		update(ui, sets, update(props, sets, 0));
	}

	void draw(graphics::CommandBuffer& out_cb, SetRefs sets, PipeRefs pipes) {
		skybox.draw(out_cb, pipes.sky, sets.sky.front());
		out_cb.bindSets(pipes.main.layout(), sets.vp.front().get(), sets.vp.front().setNumber());
		draw(ui, out_cb, sets, draw(props, out_cb, sets, 0));
	}

	static std::size_t update(PropMap& out_map, SetRefs sets, std::size_t idx) {
		for (auto& [p, props] : out_map) {
			for (auto& prop : props) {
				prop.m.set(prop.transform.model());
				prop.m.update(sets.m.at(idx), 0);
				prop.m.swap();
				prop.material->write(sets.mat.at(idx));
				++idx;
			}
		}
		return idx;
	}

	static std::size_t draw(PropMap const& map, graphics::CommandBuffer& out_cb, SetRefs sets, std::size_t idx) {
		for (auto const& [p, props] : map) {
			graphics::Pipeline& pi = p;
			out_cb.bindPipe(pi);
			for (auto const& prop : props) {
				out_cb.bindSets(pi.layout(), sets.m.at(idx).get(), sets.m.at(idx).setNumber());
				prop.material->bind(out_cb, pi, sets.mat.at(idx));
				out_cb.bindVBO(prop.mesh->vbo().buffer, &prop.mesh->ibo().buffer.get());
				if (prop.mesh->hasIndices()) {
					out_cb.drawIndexed(prop.mesh->ibo().count);
				} else {
					out_cb.draw(prop.mesh->vbo().count);
				}
				++idx;
			}
		}
		return idx;
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

class Eng {
  public:
	static vk::SurfaceKHR makeSurface(window::IInstance const& winst, vk::Instance vkinst) {
		vk::SurfaceKHR ret;
		winst.vkCreateSurface(vkinst, ret);
		return ret;
	}

	Eng(window::IInstance& winst, graphics::Bootstrap::CreateInfo const& boot, glm::ivec2 fb)
		: m_win(winst), m_boot(
							boot, [&winst](vk::Instance vi) { return makeSurface(winst, vi); }, fb),
		  m_context(m_boot.swapchain) {
	}

	Ref<window::IInstance> m_win;
	graphics::Bootstrap m_boot;
	graphics::RenderContext m_context;
};

class App {
  private:
	Token m_tk;

  public:
	App(Eng& eng, io::Reader const& reader) : m_eng(eng) {
		io::Path testV = graphics::utils::spirVpath("shaders/test.vert");
		io::Path uiV = graphics::utils::spirVpath("shaders/ui.vert");
		io::Path uiF = graphics::utils::spirVpath("shaders/ui.frag");
		io::Path testF = graphics::utils::spirVpath("shaders/test.frag");
		io::Path testFTex = graphics::utils::spirVpath("shaders/test_tex.frag");
		io::Path skyV = graphics::utils::spirVpath("shaders/skybox.vert");
		io::Path skyF = graphics::utils::spirVpath("shaders/skybox.frag");

		m_store.resources().reader(reader);
		AssetLoadInfo<graphics::Shader> ali{eng.m_boot.device, {}};
		ali.shaderPaths[graphics::Shader::Type::eVertex] = "shaders/test.vert";
		ali.shaderPaths[graphics::Shader::Type::eFragment] = "shaders/test.frag";
		[[maybe_unused]] auto sh = m_store.load<graphics::Shader>("shaders/test", ali);
		[[maybe_unused]] auto str = m_store.add<std::string>("strings/test", "Hi");
		ENSURE(m_store.contains<std::string>("strings/test"), "fubar");
		m_tk = sh->onModified.get().subscribe([]() { logI("success!"); });
		m_store.find<std::string_view>("foo");
		{
			auto const& store = m_store;
			store.find<graphics::Shader>("shaders/test");
			ENSURE(store.find<std::string>("strings/test"), "fubar");
		}
		ENSURE(m_store.unload<std::string>("strings/test"), "fubar");
		m_store.get<graphics::Shader>("shaders/test");
		auto sampler =
			m_store.add("samplers/default", graphics::Sampler{eng.m_boot.device, graphics::Sampler::info({vk::Filter::eLinear, vk::Filter::eLinear})});
		AssetLoadInfo<graphics::Texture> ali2{eng.m_boot.vram, {}, {}, {}, {}, {}, "samplers/default"};
		ali2.name = "cubemaps/sky_dusk";
		ali2.prefix = "skyboxes/sky_dusk";
		ali2.ext = ".jpg";
		ali2.imageIDs = {"right", "left", "up", "down", "front", "back"};
		auto skyboxTex = m_store.load<graphics::Texture>(ali2.name, ali2);
		m_data.tex.push_back(ali2.name);
		ali2.name = "textures/container2";
		ali2.prefix.clear();
		ali2.ext.clear();
		ali2.imageIDs = {"textures/container2.png"};
		auto containerTex = m_store.load<graphics::Texture>(ali2.name, ali2);
		m_data.tex.push_back(ali2.name);
		ali2.name = "textures/red";
		ali2.imageIDs.clear();
		ali2.raw.bytes = graphics::utils::convert({0xff, 0, 0, 0xff});
		ali2.raw.size = {1, 1};
		[[maybe_unused]] auto redTex = m_store.load<graphics::Texture>(ali2.name, ali2);
		m_data.tex.push_back(ali2.name);

		graphics::Geometry gcube = graphics::makeCube(0.5f);
		auto const skyCubeI = gcube.indices;
		auto const skyCubeV = gcube.positions();
		m_data.mesh["m0"].emplace("cube", eng.m_boot.vram, graphics::Mesh::Type::eStatic);
		m_data.mesh["m1"].emplace("cone", eng.m_boot.vram, graphics::Mesh::Type::eStatic);
		m_data.mesh["skycube"].emplace("sky_cube", eng.m_boot.vram, graphics::Mesh::Type::eStatic);
		m_data.mesh["m0"]->construct(gcube);
		m_data.mesh["m1"]->construct(graphics::makeCone());
		m_data.mesh["skycube"]->construct(Span(skyCubeV), skyCubeI);

		m_data.font.create(eng.m_boot.vram, reader, "fonts/default", "fonts/default.json", sampler.t.get().sampler(), eng.m_context.textureFormat());
		graphics::Shader test(eng.m_boot.device, {*reader.bytes(testV), *reader.bytes(testF)});
		graphics::Shader testTex(eng.m_boot.device, {*reader.bytes(testV), *reader.bytes(testFTex)});
		graphics::Shader skybox(eng.m_boot.device, {*reader.bytes(skyV), *reader.bytes(skyF)});
		graphics::Shader ui(eng.m_boot.device, {*reader.bytes(uiV), *reader.bytes(uiF)});
		m_data.pipe["test"] = eng.m_context.makePipeline("test", test, eng.m_context.pipeInfo());
		auto& pipe = *m_data.pipe["test"];
		m_data.pipe["test_tex"] = eng.m_context.makePipeline("test_tex", testTex, eng.m_context.pipeInfo(graphics::PFlags::inverse()));
		auto& pipeTex = *m_data.pipe["test_tex"];
		m_data.pipe["ui"] = eng.m_context.makePipeline("ui", ui, eng.m_context.pipeInfo(graphics::PFlags::inverse()));
		auto& pipeUI = *m_data.pipe["ui"];
		m_data.text.create(eng.m_boot.vram, "text");
		m_data.text.text.size = 80U;
		m_data.text.text.colour = colours::yellow;
		m_data.text.text.pos = {0.0f, 200.0f, 0.0f};
		m_data.text.set(m_data.font, "Hi!");

		std::array const setNums = {0U, 1U, 2U};
		m_data.sl.make("main", setNums, pipeTex);
		auto pipeSkyInfo = eng.m_context.pipeInfo();
		pipeSkyInfo.fixedState.depthStencilState.depthWriteEnable = false;
		pipeSkyInfo.fixedState.vertexInput = eng.m_context.vertexInput({0, sizeof(glm::vec3), {{vk::Format::eR32G32B32Sfloat, 0}}});
		m_data.pipe["sky"] = eng.m_context.makePipeline("skybox", skybox, pipeSkyInfo);
		m_data.sl.make("skybox", 0, *m_data.pipe["sky"]);
		m_data.cam = {0.0f, 2.0f, 4.0f};

		m_data.mat_tex.diffuse = &containerTex->t.get();
		m_data.mat_font.diffuse = &*m_data.font.atlas;
		m_data.scene.vp = graphics::TBuf<VP, false>(eng.m_boot.vram, "vp", {});
		m_data.scene.skybox.mesh = &*m_data.mesh["skycube"];
		m_data.scene.skybox.cubemap = &skyboxTex->t.get();
		auto mbuf = [&eng](std::string_view name) { return graphics::TBuf<glm::mat4, false>(eng.m_boot.vram, name, {}); };
		m_data.scene.props[pipeTex].push_back(Prop2{mbuf("prop_tex"), {}, &*m_data.mesh["m0"], &m_data.mat_tex});
		Prop2 p1{mbuf("prop_1"), {}, &*m_data.mesh["m0"], &m_data.mat_def};
		p1.transform.position({-5.0f, -1.0f, -2.0f});
		m_data.scene.props[pipe].push_back(std::move(p1));
		Prop2 p2{mbuf("prop_2"), {}, &*m_data.mesh["m1"], &m_data.mat_def};
		p2.transform.position({1.0f, -2.0f, -3.0f});
		m_data.scene.props[pipe].push_back(std::move(p2));
		m_data.scene.ui[pipeUI].push_back(Prop2{mbuf("prop_ui"), {}, &*m_data.text.mesh, &m_data.mat_font});

		eng.m_win.get().show();
	}

	void tick(Time_s dt) {
		auto const fb = m_eng.get().m_context.extent();
		m_data.scene.vp->get().mat_p = glm::perspective(glm::radians(45.0f), (f32)fb.x / std::max((f32)fb.y, 1.0f), 0.1f, 100.0f);
		{
			f32 const w = (f32)fb.x * 0.5f;
			f32 const h = (f32)fb.y * 0.5f;
			m_data.scene.vp->get().mat_ui = glm::ortho(-w, w, -h, h, -1.0f, 1.0f);
		}
		// camera
		{
			glm::vec3 const moveDir = glm::normalize(glm::cross(m_data.cam, graphics::g_nUp));
			m_data.cam += moveDir * dt.count() * 0.75f;
			m_data.scene.vp->get().mat_v = glm::lookAt(m_data.cam, {}, graphics::g_nUp);
		}
		auto& pipeTex = m_data.pipe["test_tex"];
		auto& pipe = m_data.pipe["test"];
		m_data.scene.props[*pipeTex].front().transform.rotate(glm::radians(-180.0f) * dt.count(), glm::normalize(glm::vec3(1.0f)));
		m_data.scene.props[*pipe].front().transform.rotate(glm::radians(360.0f) * dt.count(), graphics::g_nUp);
	}

	void render() {
		for (auto& tex : m_data.tex) {
			if (auto pTex = m_store.find<graphics::Texture>(tex); pTex && !pTex->t.get().ready()) {
				return;
			}
		}
		for (auto& [_, mesh] : m_data.mesh) {
			if (!mesh->ready()) {
				return;
			}
		}
		if (m_eng.get().m_context.waitForFrame()) {
			// write / update
			auto& smain = m_data.sl["main"];
			auto& ssky = m_data.sl["skybox"];
			m_data.scene.update({smain[0], smain[1], smain[2], ssky[0]});

			// draw
			if (auto r = m_eng.get().m_context.render(Colour(0x040404ff))) {
				auto& cb = r->primary();
				cb.setViewportScissor(m_eng.get().m_context.viewport(), m_eng.get().m_context.scissor());
				auto& pipe = m_data.pipe["test_tex"];
				auto& pipeSky = m_data.pipe["sky"];
				m_data.scene.draw(cb, {smain[0], smain[1], smain[2], ssky[0]}, {*pipe, *pipeSky});
				m_data.sl.swap();
			}
		}
	}

  private:
	template <typename T>
	using Map = std::unordered_map<Hash, std::optional<T>>;

	struct Data {
		Map<graphics::Mesh> mesh;
		Map<graphics::Pipeline> pipe;

		std::vector<Hash> tex;

		TexturedMaterial mat_tex;
		TexturedMaterial mat_font;
		Material mat_def;

		Font font;
		Text text;
		glm::vec3 cam;

		SetLayouts sl;
		Scene scene;
	};

	Data m_data;

  public:
	AssetStore m_store;
	Ref<Eng> m_eng;
};

bool run(CreateInfo const& info, io::Reader const& reader) {
	os::args({info.args.argc, info.args.argv});
	if (os::halt(g_cmdArgs)) {
		return 0;
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
		std::optional<Eng> eng;
		std::optional<App> app;
		bootInfo.device.pickOverride = GPUPicker::s_picked;
		Flags flags;
		time::Point t = time::now();
		while (true) {
			Time_s dt = time::now() - t;
			t = time::now();
			poll(flags, winst.pollEvents());
			if (flags.test(Flag::eClosed)) {
				break;
			}
			if (flags.test(Flag::ePaused)) {
				continue;
			}
			if (flags.test(Flag::eInit)) {
				app.reset();
				eng.emplace(winst, bootInfo, winst.framebufferSize());
				app.emplace(*eng, reader);
			}
			if (flags.test(Flag::eTerm)) {
				app.reset();
				eng.reset();
			}
			if (flags.test(Flag::eResized)) {
				/*if (!context.recreated(winst.framebufferSize())) {
					ENSURE(false, "Swapchain failure");
				}*/
				flags.reset(Flag::eResized);
			}
			if (eng && eng->m_context.reconstructed(winst.framebufferSize())) {
				continue;
			}

			if (flags.test(Flag::eDebug0)) {
				app->m_store.update();
				logD("Debug0");
				flags.reset(Flag::eDebug0);
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
