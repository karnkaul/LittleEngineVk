#pragma once
#include <vector>
#include <core/erased_ref.hpp>
#include <core/span.hpp>

namespace le::window {
struct ISurface {
	virtual Span<std::string_view> vkInstanceExtensions() const = 0;
	virtual bool vkCreateSurface(ErasedRef vkInstance, ErasedRef out_vkSurface) const = 0;
	virtual ErasedRef nativePtr() const noexcept = 0;
};
} // namespace le::window
