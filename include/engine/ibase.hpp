#pragma once

namespace le {
struct IBase {
	IBase() = default;
	IBase(IBase&&) = default;
	IBase(IBase const&) = default;
	IBase& operator=(IBase&&) = default;
	IBase& operator=(IBase const&) = default;
	virtual ~IBase() = default;
};
} // namespace le
