#pragma once
#include <le/imcpp/common.hpp>
#include <le/imcpp/input_text.hpp>
#include <le/scene/entity.hpp>
#include <le/scene/imcpp/entity_inspector.hpp>
#include <variant>

namespace le::imcpp {
class SceneInspector {
  public:
	enum class Type { eCamera, eLights };

	struct Target {
		std::variant<std::monostate, Type, Id<Entity>> payload{};

		explicit constexpr operator bool() const { return !std::holds_alternative<std::monostate>(payload); }

		constexpr auto operator==(Id<Entity> id) const -> bool {
			if (auto const* eid = std::get_if<Id<Entity>>(&payload)) { return *eid == id; }
			return false;
		}
	};

	Target target{};
	float width_pct{0.35f};

	auto display(Scene& scene) -> void;

	[[nodiscard]] auto get_entity_inspector() const -> EntityInspector& { return *m_entity_inspector; }
	auto set_entity_inspector(std::unique_ptr<EntityInspector> entity_inspector) -> void;

  private:
	void draw_to(NotClosed<Window> w, Scene& scene);

	std::unique_ptr<EntityInspector> m_entity_inspector{std::make_unique<EntityInspector>()};
};
} // namespace le::imcpp
