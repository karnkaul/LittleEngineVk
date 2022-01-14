#pragma once
#include <levk/graphics/context/device.hpp>

namespace le::graphics {
template <typename T, typename Owner = Device>
struct Deleter {
	void operator()(not_null<Owner*> owner, T t) const;
};

template <typename T, typename Owner = Device, typename D = Deleter<T, Owner>>
class Defer {
  public:
	template <typename Ty, typename De = Deleter<Ty, Device>>
	[[nodiscard]] static Defer<Ty, Device, De> make(Ty ty, not_null<Device*> device) noexcept;
	template <typename Ty, typename Ow, typename De>
	[[nodiscard]] static Defer<Ty, Ow, De> make(Ty ty, not_null<Device*> device, not_null<Ow*> owner) noexcept;

	Defer() = default;
	Defer(T t, not_null<Device*> device, not_null<Owner*> owner) noexcept : m_t(std::move(t)), m_owner(owner), m_device(device) {}

	Defer(Defer&& rhs) noexcept : Defer() { exchg(*this, rhs); }
	Defer& operator=(Defer rhs) noexcept { return (exchg(*this, rhs), *this); }
	~Defer();

	bool active() const noexcept { return m_owner && m_device && m_t != T{}; }
	T const& get() const noexcept { return m_t; }
	T& get() noexcept { return m_t; }
	operator T const&() const noexcept { return m_t; }

  private:
	static void exchg(Defer& lhs, Defer& rhs) noexcept;

	T m_t{};
	Owner* m_owner{};
	Device* m_device{};
};

template <typename D>
class Defer<void, void, D> {
  public:
	[[nodiscard]] static Defer<void, void, D> make(not_null<Device*> device) noexcept { return {device}; }

	Defer() = default;
	Defer(not_null<Device*> device) noexcept : m_device(device) {}

	Defer(Defer&& rhs) noexcept : Defer() { std::swap(m_device, rhs.m_device); }
	Defer& operator=(Defer rhs) noexcept { return (std::swap(m_device, rhs.m_device), *this); }
	~Defer();

	bool active() const noexcept { return m_device; }

  private:
	Device* m_device{};
};

// impl

template <typename T, typename Owner>
void Deleter<T, Owner>::operator()(not_null<Owner*> owner, T t) const {
	owner->destroy(t);
}

template <typename T, typename Owner, typename D>
template <typename Ty, typename De>
Defer<Ty, Device, De> Defer<T, Owner, D>::make(Ty t, not_null<Device*> device) noexcept {
	return Defer<Ty, Device, De>(std::move(t), device, device);
}

template <typename T, typename Owner, typename D>
template <typename Ty, typename Ow, typename De>
Defer<Ty, Ow, De> Defer<T, Owner, D>::make(Ty t, not_null<Device*> device, not_null<Ow*> owner) noexcept {
	return Defer<Ty, Ow, De>(std::move(t), device, owner);
}

template <typename T, typename Owner, typename D>
Defer<T, Owner, D>::~Defer() {
	if (active()) {
		m_device->defer([o = m_owner, t = std::move(m_t)] { D{}(o, t); });
	}
}

template <typename T, typename Owner, typename D>
void Defer<T, Owner, D>::exchg(Defer& lhs, Defer& rhs) noexcept {
	std::swap(lhs.m_t, rhs.m_t);
	std::swap(lhs.m_owner, rhs.m_owner);
	std::swap(lhs.m_device, rhs.m_device);
}

template <typename D>
Defer<void, void, D>::~Defer() {
	if (active()) {
		m_device->defer([] { D{}(); });
	}
}
} // namespace le::graphics
