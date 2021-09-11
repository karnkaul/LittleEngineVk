#pragma once
#include <graphics/context/device.hpp>

namespace le::graphics {
template <typename T, typename D = Device::Deleter<T>>
class Deferred final {
  public:
	Deferred() = default;
	Deferred(not_null<Device*> device, T t) : m_t(t), m_device(device) {}
	Deferred(Deferred&& rhs) noexcept : Deferred() { exchg(*this, rhs); }
	Deferred& operator=(Deferred rhs) noexcept { return (exchg(*this, rhs), *this); }
	~Deferred() {
		if (m_device) {
			m_device->defer([d = m_device, t = m_t]() { D{}(d, t); });
		}
	}
	static void exchg(Deferred& lhs, Deferred& rhs) noexcept {
		std::swap(lhs.m_t, rhs.m_t);
		std::swap(lhs.m_device, rhs.m_device);
	}

	T operator*() const noexcept { return m_t; }
	T get() const noexcept { return m_t; }
	bool active() const noexcept { return m_device != nullptr; }

	T m_t{};

  private:
	Device* m_device = {};
};

template <typename T, typename D = Device::Deleter<T>, typename... Args>
Deferred<T, D> makeDeferred(not_null<Device*> device, Args&&... args) {
	return {device, device->make<T>(std::forward<Args>(args)...)};
}
} // namespace le::graphics
