#pragma once
#include <memory>
#include <core/std_types.hpp>

namespace le {
enum class SingletonStorage : s8 { eStatic, eHeap };

template <typename T, SingletonStorage Storage = SingletonStorage::eStatic>
class Singleton {
	static_assert(alwaysFalse<T>, "Invalid Storage!");
};

namespace detail {
template <typename T>
class SingletonBase {
  public:
	static T& instance() {
		return T::inst();
	}

	static T& self() {
		return T::inst();
	}
};
} // namespace detail

template <typename T>
class Singleton<T, SingletonStorage::eStatic> : public detail::SingletonBase<T> {
  public:
	static T& inst() {
		static T s_inst;
		return s_inst;
	}
};

template <typename T>
class Singleton<T, SingletonStorage::eHeap> : public detail::SingletonBase<T> {
  public:
	static T& inst() {
		if (!s_uInst) {
			s_uInst = std::make_unique<T>();
		}
		return *s_uInst;
	}

	template <typename... Args>
	T& construct(Args&&... args) {
		s_uInst = std::make_unique<T>(std::forward<Args>(args)...);
		return *s_uInst;
	}

	bool destroy() {
		if (s_uInst) {
			s_uInst.reset();
			return true;
		}
		return false;
	}

  protected:
	static std::unique_ptr<T> s_uInst;
};

template <typename T>
std::unique_ptr<T> Singleton<T, SingletonStorage::eHeap>::s_uInst;
} // namespace le
