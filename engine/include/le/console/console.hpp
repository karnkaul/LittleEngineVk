#pragma once
#include <le/console/command.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>

namespace le::console {
struct Entry {
	std::string request{};
	Command::Response response{};
};

struct Autocomplete {
	std::string common_suffix{};
	std::vector<std::string_view> candidates{};
};

class Console {
  public:
	static constexpr std::size_t max_entries_v{100};
	static constexpr std::size_t max_log_entries_v{100};
	static constexpr std::size_t max_history_v{100};

	Console();

	template <std::derived_from<Command> Type, typename... Args>
		requires(std::constructible_from<Type, Args...>)
	auto add(Args&&... args) -> Type& {
		auto t = std::make_unique<Type>(std::forward<Args>(args)...);
		auto& ret = *t;
		auto name = static_cast<Command const&>(ret).get_name();
		m_commands.insert_or_assign(name, std::move(t));
		return ret;
	}

	void submit(std::string_view text);
	[[nodiscard]] auto autocomplete(std::string_view text, std::size_t cursor) const -> Autocomplete;

	void add_entry(Entry entry);
	void add_entry(Autocomplete const& ac);
	void add_to_history(std::string text);

	[[nodiscard]] auto get_entries() const -> std::deque<Entry> const& { return m_entries; }
	[[nodiscard]] auto get_history() const -> std::deque<std::string> const& { return m_history; }

	void clear_log() { m_entries.clear(); }
	void clear_history() { m_history.clear(); }

	std::size_t max_entries{max_log_entries_v};
	std::size_t max_log_entries{max_log_entries_v};
	std::size_t max_history{max_history_v};

  private:
	struct Builtin {
		std::string_view name{};
		std::function<void(Console&)> command{};
	};

	[[nodiscard]] auto autocomplete(std::string_view fragment) const -> std::vector<std::string_view>;
	auto builtin(std::string_view cmd) -> bool;
	[[nodiscard]] auto get_builtin_names(std::string_view fragment) const -> std::vector<std::string_view>;

	std::unordered_map<std::string_view, std::unique_ptr<Command>> m_commands{};
	std::vector<Builtin> m_builtins{};
	std::deque<Entry> m_entries{};
	std::deque<std::string> m_history{};
};
} // namespace le::console
