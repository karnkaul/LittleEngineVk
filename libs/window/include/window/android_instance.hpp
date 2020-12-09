#pragma once
#if defined(__ANDROID__)
#include <window/instance.hpp>

namespace le::window {
class AndroidInstance final : public IInstance {
  public:
	struct CreateInfo;

	AndroidInstance() : IInstance(false) {
	}
	explicit AndroidInstance(CreateInfo const& info);
	AndroidInstance(AndroidInstance&&) = default;
	AndroidInstance& operator=(AndroidInstance&&) = default;
	~AndroidInstance();

	// IISurface
	Span<std::string_view> vkInstanceExtensions() const override;
	bool vkCreateSurface(ErasedRef vkInstance, ErasedRef out_vkSurface) const override;
	ErasedRef nativePtr() const noexcept override;

	// Instance
	EventQueue pollEvents() override;
};

struct AndroidInstance::CreateInfo {
	struct {
		ErasedRef androidApp;
	} config;
	struct {
		LibLogger::Verbosity verbosity = LibLogger::Verbosity::eEndUser;
	} options;
};
} // namespace le::window
#endif
