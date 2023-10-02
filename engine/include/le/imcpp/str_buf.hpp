#pragma once
#include <string_view>
#include <vector>

struct ImGuiInputTextCallbackData;

namespace le::imcpp {
struct StrBuf {
	static constexpr std::size_t buf_size_v{32};

	std::vector<char> buf{std::vector<char>(buf_size_v, '\0')};

	[[nodiscard]] auto view() const -> std::string_view { return buf.data(); }
	[[nodiscard]] auto size() const -> std::size_t { return view().size(); }
	[[nodiscard]] auto capacity() const -> std::size_t { return buf.size(); }

	void extend(ImGuiInputTextCallbackData& data, std::size_t count = 1);
	void clear();
};
} // namespace le::imcpp
