#pragma once
#include <levk/core/kassert/src_loc.hpp>
#include <levk/core/utils/debug.hpp>
#include <string_view>

#define KASSERT_MSG(pred, msg)                                                                                                                                 \
	do {                                                                                                                                                       \
		if (!(pred)) {                                                                                                                                         \
			auto const ctx = ::le::AssertData<>{#pred, msg, ::le::SrcLoc::current()};                                                                          \
			::le::kassertNotify(ctx);                                                                                                                          \
			DEBUG_TRAP();                                                                                                                                      \
			::le::kassertTrigger(ctx);                                                                                                                         \
		}                                                                                                                                                      \
	} while (false)

#define KASSERT_NOMSG(pred) KASSERT_MSG(pred, "")

#define KASSERT_RESOLVE(_1, _2, NAME, ...) NAME
#define KASSERT(...) KASSERT_RESOLVE(__VA_ARGS__, KASSERT_MSG, KASSERT_NOMSG)(__VA_ARGS__)

namespace le {
template <typename Text = std::string_view>
struct AssertData {
	Text expression{};
	Text message{};
	SrcLoc location{};
};

extern void kassertNotify(AssertData<> const& data);
extern void kassertTrigger(AssertData<> const& data) noexcept(false);
} // namespace le
