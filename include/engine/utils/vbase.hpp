#pragma once

namespace le::utils {
struct VBase {
	VBase() = default;
	VBase(VBase&&) = default;
	VBase(VBase const&) = default;
	VBase& operator=(VBase&&) = default;
	VBase& operator=(VBase const&) = default;
	virtual ~VBase() = default;
};
} // namespace le::utils
