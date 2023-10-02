#pragma once
#include <le/core/polymorphic.hpp>
#include <string>

namespace le::console {
class Command : public Polymorphic {
  public:
	struct Response {
		std::string message{};
		std::string error{};
	};

	explicit Command(std::string name) : m_name(std::move(name)) {}

	[[nodiscard]] auto get_name() const -> std::string_view { return m_name; }

	virtual auto process(std::string_view argument) -> Response = 0;

  private:
	std::string m_name{};
};
} // namespace le::console
