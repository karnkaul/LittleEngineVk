#pragma once
#include <core/utils/debug.hpp>
#include <core/utils/src_info.hpp>

#if defined(EXPECT)
#undef EXPECT
#endif
#define EXPECT(pred)                                                                                                                                           \
	if (!(pred)) {                                                                                                                                             \
		::le::utils::SrcInfo{__func__, __FILE__, __LINE__}.logMsg("Expectation failed: ", #pred, ::dlog::level::warn);                                         \
		DEBUG_TRAP();                                                                                                                                          \
	}
