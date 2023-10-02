#include <imgui.h>
#include <le/imcpp/str_buf.hpp>
#include <cassert>

namespace le::imcpp {
void StrBuf::extend(ImGuiInputTextCallbackData& data, std::size_t count) {
	buf.resize(buf.size() + count, '\0');
	data.BufSize = static_cast<int>(buf.size());
	data.Buf = buf.data();
}

void StrBuf::clear() {
	assert(!buf.empty());
	buf.front() = '\0';
}
} // namespace le::imcpp
