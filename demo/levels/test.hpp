#pragma once
#include <core/log.hpp>
#include <engine/game/state.hpp>
#include <level.hpp>

class TestLevel final : public Level {
  public:
	using Deg = le::f32;

  public:
	TestLevel();

  public:
	void tick(le::Time dt) override;

  private:
	struct {
		le::ecs::Entity mainText;
		le::ecs::Entity elapsedText;
		le::Time elapsed;
	} m_data;

	struct {
		le::Prop ship;
		le::f32 roll = 0.0f;
		Deg maxRoll = 45.0f;
		glm::quat orientTarget = le::gfx::g_qIdentity;
		bool bControl = true;
	} m_game;
};
