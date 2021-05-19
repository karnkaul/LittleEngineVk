#pragma once
#include <vector>
#include <core/erased_ptr.hpp>
#include <core/span.hpp>

namespace le::window {
struct ISurface {
	virtual View<std::string_view> vkInstanceExtensions() const = 0;
	virtual bool vkCreateSurface(ErasedPtr vkInstance, ErasedPtr vkSurface) const = 0;
	virtual ErasedPtr nativePtr() const noexcept = 0;
};
} // namespace le::window
