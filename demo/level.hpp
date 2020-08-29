#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <engine/game/input_context.hpp>
#include <engine/game/driver.hpp>

namespace stdfs = std::filesystem;

class Level
{
protected:
	std::string m_name;

protected:
	struct
	{
		stdfs::path id;
		le::input::Context context;
		le::input::Token token;
	} m_input;
	struct
	{
		stdfs::path id;
		le::res::Token token;
	} m_manifest;

public:
	Level();
	virtual ~Level();

protected:
	virtual void tick(le::Time dt) = 0;

protected:
	virtual le::SceneBuilder const& builder() const;
	virtual void onManifestLoaded();

protected:
	le::Registry const& registry() const;
	le::Registry& registry();
	le::gfx::Camera const& camera() const;
	le::gfx::Camera& camera();
	le::gfx::ScreenRect const& gameRect() const;
	le::gfx::ScreenRect& gameRect();

private:
	friend class LevelDriver;
};

class LevelDriver final : public le::engine::Driver
{
private:
	std::unique_ptr<Level> m_active;
	bool m_bTicked = false;

public:
	void tick(le::Time dt) override;
	le::SceneBuilder const& builder() const override;

	void cleanup();

private:
	void unloadLoad();
	void perFrame();
};

inline LevelDriver g_driver;
