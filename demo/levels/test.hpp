#pragma once
#include <core/log.hpp>
#include <level.hpp>

class TestLevel final : public Level
{
private:
	struct
	{
		le::Entity mainText;
		le::Entity elapsedText;
		le::Time elapsed;
	} m_data;

public:
	TestLevel()
	{
		using namespace le;
		m_name = "Test";
		m_manifest.id = "test.manifest";

		m_input.context.mapTrigger("load_prev", [this]() {
			// if (!loadWorld(m_previousWorldID))
			{
				LOG_W("[{}] Not implemented", m_name);
			}
		});
		m_input.context.addTrigger("load_prev", input::Key::eP, input::Action::eRelease, input::Mods::eCONTROL);

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
	}

public:
	void tick(le::Time dt) override
	{
		m_data.elapsed += dt;
		registry().component<le::UIComponent>(m_data.elapsedText)->setText(fmt::format("{:.1f}", m_data.elapsed.to_s()));
	}
};
