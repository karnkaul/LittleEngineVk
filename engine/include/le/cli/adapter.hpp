#pragma once
#include <le/core/string_trie.hpp>

namespace le::cli {
class Adapter {
  public:
	Adapter() = default;
	Adapter(Adapter const&) = default;
	Adapter(Adapter&&) = default;
	auto operator=(Adapter const&) -> Adapter& = default;
	auto operator=(Adapter&&) -> Adapter& = default;

	virtual ~Adapter() = default;

	virtual auto get_response(std::string_view command) -> std::string = 0;
	[[nodiscard]] virtual auto get_command_trie() const -> StringTrie = 0;
};
} // namespace le::cli
