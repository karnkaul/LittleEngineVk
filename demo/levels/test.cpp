#include <core/maths.hpp>
#include <engine/resources/resources.hpp>
#include <levels/test.hpp>
#include <level.hpp>

using namespace le;

TestLevel::TestLevel()
{
	m_name = "Test";
	m_manifest.id = "test.manifest";

	m_input.context.mapTrigger("load_prev", [this]() {
		// if (!loadWorld(m_previousWorldID))
		{
			LOG_W("[{}] Not implemented", m_name);
		}
	});
	m_input.context.addTrigger("load_prev", input::Key::eP, input::Action::eRelease, input::Mods::eCONTROL);
	m_input.context.mapRange("roll", [this](f32 value) {
		if (m_game.bControl)
		{
			m_game.roll = std::clamp(-value, -1.0f, 1.0f);
		}
	});
	m_input.context.addRange("roll", input::Key::eA, input::Key::eD);
	m_input.context.addRange("roll", input::Key::eLeft, input::Key::eRight);
	m_input.context.addRange("roll", input::Axis::eLeftX);

	m_data.mainText = registry().spawnEntity<UIComponent>("mainText");
	m_data.elapsedText = registry().spawnEntity<UIComponent>("elapsedText");
	Text2D::Info info;
	info.data.colour = colours::white;
	info.data.text = "Test World";
	info.data.scale = 0.25f;
	info.id = "title";
	registry().component<UIComponent>(m_data.mainText)->setText(std::move(info));
	info.data.text = "0";
	info.data.pos = {0.0f, -100.0f, 0.0f};
	info.id = "elapsed";
	m_data.elapsed = {};
	registry().component<UIComponent>(m_data.elapsedText)->setText(std::move(info));
	camera().position = {0.0f, 1.0f, 2.0f};

	m_game.ship = gs::spawnProp("ship");
	m_game.ship.transform().position({0.0f, 0.0f, -3.0f});
	*registry().addComponent<res::Mesh>(m_game.ship.entity) = *res::find<res::Mesh>("meshes/cube");

	registry().spawnEntity<SceneDesc>("scene_desc");
}

void TestLevel::tick(Time dt)
{
	m_data.elapsed += dt;
	registry().component<UIComponent>(m_data.elapsedText)->setText(fmt::format("{:.1f}", m_data.elapsed.to_s()));

	auto pTransform = registry().component<Transform>(m_game.ship);
	glm::quat const orientTarget = glm::rotate(gfx::g_qIdentity, glm::radians(m_game.roll * m_game.maxRoll), gfx::g_nFront);
	m_game.orientTarget = glm::slerp(m_game.orientTarget, orientTarget, dt.to_s() * 10);
	pTransform->orient(m_game.orientTarget);

	m_game.roll = maths::lerp(m_game.roll, 0.0f, dt.to_s() * 10);
}
