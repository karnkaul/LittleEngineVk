#pragma once
#include <graphics/context/device.hpp>

namespace le::graphics {
template <typename T, typename D = Device::Deleter<T>>
class Deferred {
  public:
	Deferred() = default;
	Deferred(not_null<Device*> device, T t) : m_t(t), m_device(device) {}
	Deferred(Deferred&& rhs) noexcept : m_t(rhs.m_t), m_device(std::exchange(rhs.m_device, nullptr)) {}
	Deferred& operator=(Deferred&& rhs) {
		if (&rhs != this) {
			destroy();
			m_t = rhs.m_t;
			m_device = std::exchange(rhs.m_device, nullptr);
		}
		return *this;
	}
	~Deferred() { destroy(); }

	T operator*() const noexcept { return m_t; }
	T get() const noexcept { return m_t; }
	bool active() const noexcept { return m_device != nullptr; }

	T m_t;

  private:
	Device* m_device = {};

	void destroy() {
		if (m_device) {
			m_device->defer([d = m_device, t = m_t]() { D{}(d, t); });
		}
		m_t = T{};
		m_device = {};
	}
};

template <typename T, typename... Args>
using DeviceFn = T (Device::*)(Args...) const;

template <typename T, typename D = Device::Deleter<T>, typename... Args>
Deferred<T, D> makeDeferred(not_null<Device*> device, Args&&... args) {
	return {device, device->make<T>(std::forward<Args>(args)...)};
}
} // namespace le::graphics
