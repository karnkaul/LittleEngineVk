#pragma once
#include <le/imcpp/common.hpp>
#include <le/imcpp/input_text.hpp>
#include <le/scene/entity.hpp>
#include <variant>

namespace le::imcpp {
class Inspector {
  public:
	enum class Type { eSceneCamera, eLights };

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

	void display(Scene& scene);

  private:
	struct EntityName {
		std::optional<Id<Entity>> previous{};
		imcpp::InputText<> input_text{};
	};

	void draw_to(NotClosed<Window> w, Scene& scene);
	auto get_entity_name(Id<Entity> id, std::string_view name) -> imcpp::InputText<>&;

	EntityName m_entity_name{};
};
} // namespace le::imcpp
