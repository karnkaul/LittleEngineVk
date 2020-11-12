#include <core/maths.hpp>
#include <engine/game/spring_arm.hpp>
#include <engine/resources/resources.hpp>
#include <level.hpp>
#include <levels/test.hpp>

using namespace le;

TestLevel::TestLevel() {
	m_name = "Test";
	m_manifest.id = "test.manifest";

	m_input.context.mapTrigger("load_prev", [this]() {
		// if (!loadWorld(m_previousWorldID))
		{ logW("[{}] Not implemented", m_name); }
	});
	m_input.context.addTrigger("load_prev", input::Key::eP, input::Action::eRelease, input::Mods::eCONTROL);
	m_input.context.mapRange("roll", [this](f32 value) {
		if (m_game.bControl) {
			m_game.roll = std::clamp(-value, -1.0f, 1.0f);
			if (maths::abs(m_game.roll) < 0.25f) {
				m_game.roll = 0.0f;
			} else if (maths::abs(m_game.roll) > 0.9f) {
				m_game.roll = (m_game.roll < 0.0f) ? -1.0f : 1.0f;
			}
		}
	});
	m_input.context.addRange("roll", input::Key::eA, input::Key::eD);
	m_input.context.addRange("roll", input::Key::eLeft, input::Key::eRight);
	m_input.context.addRange("roll", input::Axis::eLeftX);

	auto mainText = registry().spawn<UIComponent>("mainText");
	m_data.mainText = mainText.entity;
	auto elapsedText = registry().spawn<UIComponent>("elapsedText");
	m_data.elapsedText = elapsedText.entity;
	Text2D::Info info;
	info.data.colour = colours::white;
	info.data.text = "Test World";
	info.data.size = 0.25f;
	info.id = "title";
	auto& [title] = mainText.components;
	title.setText(std::move(info));
	info.data.text = "0";
	info.data.pos = {0.0f, -100.0f, 0.0f};
	info.id = "elapsed";
	m_data.elapsed = {};
	auto& [elapsed] = elapsedText.components;
	elapsed.setText(std::move(info));
	gs::g_game.mainCamera().position = {0.0f, 1.0f, 2.0f};

	m_game.ship = gs::g_game.spawnProp("ship");
	m_game.ship.transform().position({0.0f, 0.0f, -3.0f});
	*registry().attach<res::Mesh>(m_game.ship.entity) = *res::find<res::Mesh>("meshes/cube");
	if (auto pSpring = registry().attach<SpringArm>(m_game.ship.entity)) {
		pSpring->pTarget = m_game.ship.pTransform;
		pSpring->position = gs::g_game.mainCamera().position;
		pSpring->k = 0.5f;
		pSpring->b = 0.05f;
		pSpring->offset = pSpring->position - m_game.ship.transform().position();
	}
}

void TestLevel::tick(Time_s dt) {
	m_data.elapsed += dt;
	if (auto pUI = registry().find<UIComponent>(m_data.elapsedText)) {
		pUI->setText(fmt::format("{:.1f}", m_data.elapsed.count()));
	}
	if (auto pTransform = registry().find<Transform>(m_game.ship)) {
		glm::quat const orientTarget = glm::rotate(gfx::g_qIdentity, glm::radians(m_game.roll * m_game.maxRoll), gfx::g_nFront);
		m_game.orientTarget = glm::slerp(m_game.orientTarget, orientTarget, dt.count() * 10);
		pTransform->orient(m_game.orientTarget);
		glm::vec3 const dpos = -m_game.roll * dt.count() * 10 * gfx::g_nRight;
		pTransform->position(pTransform->position() + dpos);
		if (auto pSpringArm = registry().find<SpringArm>(m_game.ship)) {
			gs::g_game.mainCamera().position = pSpringArm->tick(dt);
		}
	}
	m_game.roll = maths::lerp(m_game.roll, 0.0f, dt.count() * 10);
}
