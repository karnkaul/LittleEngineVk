#pragma once
#include <le/imcpp/common.hpp>
#include <le/imcpp/input_text.hpp>
#include <le/scene/entity.hpp>

namespace le::imcpp {
class EntityInspector {
  public:
	EntityInspector() = default;
	EntityInspector(EntityInspector const&) = default;
	EntityInspector(EntityInspector&&) = default;
	auto operator=(EntityInspector const&) -> EntityInspector& = default;
	auto operator=(EntityInspector&&) -> EntityInspector& = default;

	virtual ~EntityInspector() = default;

	auto inspect(OpenWindow w, Entity& out) -> void;

  private:
	struct EntityName {
		std::optional<Id<Entity>> previous{};
		imcpp::InputText<> input_text{};
	};

	virtual auto inspect_components(OpenWindow w, Entity& out) -> void;

	auto get_entity_name(Id<Entity> id, std::string_view name) -> imcpp::InputText<>&;

	EntityName m_entity_name{};
};
} // namespace le::imcpp
