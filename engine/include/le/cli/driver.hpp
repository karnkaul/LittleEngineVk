#pragma once
#include <le/cli/responder.hpp>
#include <memory>
#include <unordered_map>

namespace le::cli {
class Driver : public Responder {
  public:
	void autocomplete(Console& console) const final;
	void submit(Console& console) final;

	template <std::derived_from<Command> CmdT, typename... Args>
		requires(std::constructible_from<CmdT, Args...>)
	auto add_command(Args&&... args) -> CmdT& {
		auto t = std::make_unique<CmdT>(std::forward<Args>(args)...);
		auto& ret = *t;
		add_command(std::move(t));
		return ret;
	}

  private:
	void add_command(std::unique_ptr<Command> sub_command);

	std::unordered_map<std::string_view, std::unique_ptr<Command>> m_commands{};
};
} // namespace le::cli
