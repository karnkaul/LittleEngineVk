#include <core/log.hpp>
#include <core/threads.hpp>
#include <core/time.hpp>
#include <engine/assets/resources.hpp>
#include <engine/gfx/font.hpp>
#include <engine/gfx/mesh.hpp>
#include <engine/gfx/texture.hpp>
#include <engine/levk.hpp>
#include <core/utils.hpp>

namespace le
{
std::string const Resources::s_tName = utils::tName<Resources>();

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

bool Resources::init(io::Reader const& data)
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
	if (!m_bActive.load())
	{
		auto semaphore = setBusy();
		m_bActive.store(true);
		{
			create<gfx::Material>("materials/default", {});
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
			gfx::Font::Info fontInfo;
			auto [str, bResult] = data.getString("fonts/default.json");
			ASSERT(bResult, "Default font not found!");
			GData fontData;
			if (fontData.read(std::move(str)))
			{
				fontInfo.deserialise(fontData);
				auto [img, bImg] = data.getBytes(stdfs::path("fonts") / fontInfo.sheetID);
				ASSERT(bImg, "Default font not found!");
				fontInfo.image = std::move(img);
				fontInfo.material.pMaterial = get<gfx::Material>(fontInfo.materialID);
				create<gfx::Font>("fonts/default", std::move(fontInfo));
			}
			else
			{
				LOG_E("[{}] Failed to create default font!", s_tName);
			}
		}
	}
	LOG_I("[{}] initialised", s_tName);
	return true;
}

void Resources::update()
{
	ASSERT(m_bActive.load(), "Resources inactive!");
	if (m_bActive.load())
	{
		auto lock = m_mutex.lock();
		for (auto& [id, uResource] : m_resources.m_map)
		{
			uResource->update();
		}
	}
	return;
}

bool Resources::unload(Hash hash)
{
	ASSERT(m_bActive.load(), "Resources inactive!");
	if (m_bActive.load())
	{
		auto lock = m_mutex.lock();
		return m_resources.unload(hash);
	}
	return false;
}

void Resources::deinit()
{
	waitIdle();
	ASSERT(m_counter.isZero(false), "Resources in use!");
	auto lock = m_mutex.lock();
	m_resources.unloadAll();
	if (m_bActive.load())
	{
		m_bActive.store(false);
	}
	LOG_I("[{}] deinitialised", s_tName);
	return;
}

Resources::Semaphore Resources::setBusy() const
{
	return Semaphore(m_counter);
}

void Resources::waitIdle()
{
	constexpr Time timeout = 5s;
	Time elapsed;
	Time const start = Time::elapsed();
	while (!m_counter.isZero(true) && elapsed < timeout)
	{
		elapsed = Time::elapsed() - start;
		threads::sleep();
	}
	bool bTimeout = elapsed > timeout;
	ASSERT(!bTimeout, "Timeout waiting for Resources! Expect a crash");
	LOGIF_E(bTimeout, "[{}] Timeout waiting for Resources! Expect crashes/hangs!", s_tName);
}
} // namespace le
