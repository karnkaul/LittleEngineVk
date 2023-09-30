#pragma once
#include <le/console/command.hpp>

namespace le::console {
class Trigger : public Command {
  public:
	using Base = Trigger;

	using Command::Command;

	virtual void on_triggered() = 0;

  private:
	auto process(std::string_view const /*unused*/) -> Response final {
		on_triggered();
		return {};
	}
};
} // namespace le::console
