#pragma once
#include <spaced/engine/error.hpp>

namespace spaced {
template <typename Type>
class MonoInstance {
  public:
	MonoInstance(MonoInstance const&) = delete;
	MonoInstance(MonoInstance&&) = delete;

	auto operator=(MonoInstance const&) -> MonoInstance& = delete;
	auto operator=(MonoInstance&&) -> MonoInstance& = delete;

	MonoInstance() {
		if (s_instance != nullptr) { throw Error{"Duplicate instance"}; }
		s_instance = static_cast<Type*>(this);
	}

	~MonoInstance() { s_instance = nullptr; }

	static auto instance() -> Type& {
		if (!s_instance) { throw Error{"Nonexistent instance"}; }
		return *s_instance;
	}

	static auto self() -> Type& { return instance(); }

  protected:
	// NOLINTNEXTLINE
	inline static Type* s_instance{};
};
} // namespace spaced
