#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <engine/game/input_context.hpp>
#include <engine/game/driver.hpp>

namespace stdfs = std::filesystem;

class Level
{
public:
	Level();
	virtual ~Level();

protected:
	virtual void tick(le::Time dt) = 0;

protected:
	virtual le::SceneBuilder const& builder() const;
	virtual void onManifestLoaded();

protected:
	le::ecs::Registry const& registry() const;
	le::ecs::Registry& registry();

private:
	friend class LevelDriver;

protected:
	std::string m_name;

protected:
	struct
	{
		stdfs::path id;
		le::input::Context context;
		le::Token token;
	} m_input;
	struct
	{
		stdfs::path id;
		le::Token token;
	} m_manifest;
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
