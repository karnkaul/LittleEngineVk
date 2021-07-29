#pragma once
#include <string_view>
#include <type_traits>
#include <core/delegate.hpp>
#include <core/ensure.hpp>
#include <core/not_null.hpp>

namespace le {
template <typename T>
class Asset {
  public:
	using type = T;
	using OnModified = Delegate<>;

	Asset() = default;
	Asset(not_null<type*> t, std::string_view id, OnModified* onMod = {}) noexcept;

	operator Asset<T const>() const noexcept { return {m_t, m_id, m_onModified}; }

	bool valid() const noexcept { return m_t != nullptr; }
	explicit operator bool() const noexcept { return valid(); }
	bool operator==(Asset<T> const& rhs) const noexcept { return m_id == rhs.m_id && m_t == rhs.m_t; }

	T* peek() const noexcept { return m_t; }
	T& get() const;
	T& operator*() const { return get(); }
	T* operator->() const { return &get(); }
	OnModified::Tk onModified(OnModified::Callback const& callback);

	std::string_view m_id;

  private:
	T* m_t{};
	OnModified* m_onModified{};
};

// impl

template <typename T>
Asset<T>::Asset(not_null<type*> t, std::string_view id, OnModified* onMod) noexcept : m_id(id), m_t(t), m_onModified(onMod) {}
template <typename T>
T& Asset<T>::get() const {
	ensure(valid(), "Invalid asset");
	return *m_t;
}
template <typename T>
typename Asset<T>::OnModified::Tk Asset<T>::onModified(OnModified::Callback const& callback) {
	return m_onModified ? m_onModified->subscribe(callback) : OnModified::Tk();
}
} // namespace le
