#pragma once
#include <le/imcpp/common.hpp>
#include <array>
#include <cstring>
#include <string_view>

namespace le::imcpp {
template <std::size_t Capacity = 128>
struct InputText {
	std::array<char, Capacity> buffer{};
	std::size_t used{};

	auto operator()(char const* label, int flags = {}) -> bool {
		auto ret = input_text(label, buffer.data(), buffer.size(), flags);
		used = std::string_view{buffer.data()}.size();
		return ret;
	}

	void set(std::string_view text) {
		if (text.empty()) {
			*this = {};
			return;
		}
		used = std::min(text.size(), buffer.size());
		std::memcpy(buffer.data(), text.data(), used);
	}

	[[nodiscard]] constexpr auto empty() const -> bool { return used == 0; }
	[[nodiscard]] constexpr auto view() const -> std::string_view { return {buffer.data(), used}; }

	constexpr operator std::string_view() const { return view(); }
};
} // namespace le::imcpp
