#pragma once
#include <memory>
#include <string>
#include <core/io/path.hpp>
#include <engine/game/driver.hpp>
#include <engine/game/input_context.hpp>

class Level {
  public:
	Level();
	virtual ~Level();

  protected:
	virtual void tick(le::Time_s dt) = 0;

  protected:
	virtual le::SceneBuilder const& builder() const;
	virtual void onManifestLoaded();

  protected:
	decf::registry_t const& registry() const;
	decf::registry_t& registry();

  private:
	friend class LevelDriver;

  protected:
	std::string m_name;

  protected:
	struct {
		le::io::Path id;
		le::input::Context context;
		le::Token token;
	} m_input;
	struct {
		le::io::Path id;
		le::Token token;
	} m_manifest;
};

class LevelDriver final : public le::engine::Driver {
  private:
	std::unique_ptr<Level> m_active;
	bool m_bTicked = false;

  public:
	void tick(le::Time_s dt) override;
	le::SceneBuilder const& builder() const override;

	void cleanup();

  private:
	void unloadLoad();
	void perFrame();
};

inline LevelDriver g_driver;
