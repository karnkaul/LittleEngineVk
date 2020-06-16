#include <core/log.hpp>
#include <core/threads.hpp>
#include <engine/assets/resources.hpp>
#include <engine/gfx/font.hpp>
#include <engine/gfx/mesh.hpp>
#include <engine/gfx/shader.hpp>
#include <engine/gfx/texture.hpp>
#include <engine/levk.hpp>
#include <core/utils.hpp>

namespace le
{
std::string const Resources::s_tName = utils::tName<Resources>();

Resources& Resources::inst()
{
	static Resources s_inst;
	return s_inst;
}

Resources::Resources()
{
	m_bActive.store(false);
}

Resources::~Resources()
{
	ASSERT(!m_bActive.load() && m_resources.count() == 0, "Resources singleton not deinitialised!");
	if (m_bActive.load() || m_resources.count() > 0)
	{
		deinit();
	}
}

bool Resources::init(IOReader const& data)
{
	constexpr static std::array<u8, 4> white1pxBytes = {0xff, 0xff, 0xff, 0xff};
	constexpr static std::array<u8, 4> black1pxBytes = {0x0, 0x0, 0x0, 0x0};
	static std::array const shaderIDs = {stdfs::path("shaders/uber.vert"), stdfs::path("shaders/uber.frag")};
	static stdfs::path const fontID = "fonts/default.json";
	if (!data.checkPresences(shaderIDs) || !data.checkPresence(fontID))
	{
		LOG_E("[{}] Failed to locate required shaders/fonts!", s_tName);
		return false;
	}
	std::scoped_lock<std::mutex> lock(m_mutex); // block deinit()
	if (!m_bActive.load())
	{
		m_bActive.store(true);
		{
			create<gfx::Material>("materials/default", {});
		}
		{
			create<gfx::Sampler>("samplers/default", {});
			gfx::Sampler::Info info;
			info.mode = gfx::Sampler::Mode::eClampEdge;
			info.min = gfx::Sampler::Filter::eNearest;
			info.mip = gfx::Sampler::Filter::eNearest;
			create<gfx::Sampler>("samplers/font", std::move(info));
		}
		{
			gfx::Mesh::Info info;
			info.geometry = gfx::createCube();
			create<gfx::Mesh>("meshes/cube", std::move(info));
		}
		{
			gfx::Texture::Info info;
			info.type = gfx::Texture::Type::e2D;
			info.raws = {{Span<u8>(white1pxBytes), {1, 1}}};
			create<gfx::Texture>("textures/white", info);
			info.raws.back().bytes = Span<u8>(black1pxBytes);
			create<gfx::Texture>("textures/black", info);
		}
		{
			gfx::Texture::Info info;
			gfx::Texture::Raw b1px;
			b1px.bytes = Span<u8>(black1pxBytes);
			b1px.size = {1, 1};
			info.raws = {b1px, b1px, b1px, b1px, b1px, b1px};
			info.type = gfx::Texture::Type::eCube;
			create<gfx::Texture>("cubemaps/blank", std::move(info));
		}
		{
			gfx::Shader::Info info;

			info.pReader = &data;
			ASSERT(data.checkPresences(shaderIDs), "Uber Shader not found!");
			info.codeIDMap.at((size_t)gfx::Shader::Type::eVertex) = shaderIDs.at(0);
			info.codeIDMap.at((size_t)gfx::Shader::Type::eFragment) = shaderIDs.at(1);
			create<gfx::Shader>("shaders/default", std::move(info));
		}
		{
			gfx::Font::Info fontInfo;
			auto [str, bResult] = data.getString("fonts/default.json");
			ASSERT(bResult, "Default font not found!");
			fontInfo.deserialise(GData(std::move(str)));
			auto [img, bImg] = data.getBytes(stdfs::path("fonts") / fontInfo.sheetID);
			ASSERT(bImg, "Default font not found!");
			fontInfo.image = std::move(img);
			create<gfx::Font>("fonts/default", std::move(fontInfo));
		}
	}
	return true;
}

void Resources::update()
{
	ASSERT(m_bActive.load(), "Resources inactive!");
	std::scoped_lock<std::mutex> lock(m_mutex);
	if (m_bActive.load())
	{
		for (auto& [id, uResource] : m_resources.m_map)
		{
			uResource->update();
		}
	}
	return;
}

void Resources::deinit()
{
	std::unique_lock<std::shared_mutex> semLock(m_semaphore); // block create()
	std::scoped_lock<std::mutex> lock(m_mutex);				  // block init()
	if (m_bActive.load())
	{
		m_bActive.store(false);
		m_resources.unloadAll();
	}
	return;
}
} // namespace le
