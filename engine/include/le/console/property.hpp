#pragma once
#include <le/console/command.hpp>
#include <charconv>
#include <concepts>
#include <format>

namespace le::console {
template <typename Type>
concept PropertyT = std::same_as<Type, bool> || std::integral<Type> || std::floating_point<Type> || std::same_as<Type, std::string>;

template <PropertyT Type>
constexpr auto to_str() {
	if constexpr (std::same_as<Type, bool>) {
		return "boolean";
	} else if constexpr (std::integral<Type>) {
		return "integer";
	} else if constexpr (std::floating_point<Type>) {
		return "float";
	} else {
		return "string";
	}
}

template <PropertyT Type>
class Property : public Command {
  public:
	using Base = Property<Type>;

	using Command::Command;

	virtual void on_changed() = 0;

	Type value{};

  private:
	auto process(std::string_view const argument) -> Response final {
		if (argument.empty()) { return Response{.message = std::format("{}", value)}; }

		auto invalid_arg = [&] { return Response{.error = std::format("invalid argument, expected {}", to_str<Type>())}; };

		if constexpr (std::same_as<Type, bool>) {
			if (argument == "true" || argument == "1") {
				value = true;
			} else if (argument == "false" || argument == "0") {
				value = false;
			} else {
				return invalid_arg();
			}
		} else if constexpr (std::same_as<Type, std::string>) {
			value = argument;
		} else {
			auto const* end = argument.data() + argument.size();
			auto [ptr, ec] = std::from_chars(argument.data(), end, value);
			if (ec != std::errc{} || ptr != end) { return invalid_arg(); }
		}

		on_changed();
		return {};
	}
};
} // namespace le::console
