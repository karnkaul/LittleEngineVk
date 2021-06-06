#pragma once
#if defined(LEVK_ANDROID)
#include <window/instance.hpp>

namespace le::window {
class AndroidInstance final : public IInstance {
  public:
	AndroidInstance() : IInstance(false) {}
	explicit AndroidInstance(CreateInfo const& info);
	AndroidInstance(AndroidInstance&&) = default;
	AndroidInstance& operator=(AndroidInstance&&) = default;
	~AndroidInstance();

	// IISurface
	Span<std::string_view const> vkInstanceExtensions() const override;
	bool vkCreateSurface(ErasedPtr vkInstance, ErasedPtr vkSurface) const override;
	ErasedPtr nativePtr() const noexcept override;

	// Instance
	EventQueue pollEvents() override;
};
} // namespace le::window
#endif
