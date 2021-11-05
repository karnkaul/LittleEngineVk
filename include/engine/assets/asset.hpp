#pragma once
#include <string_view>
#include <type_traits>
#include <core/not_null.hpp>
#include <core/utils/error.hpp>
#include <ktl/delegate.hpp>

namespace le {
template <typename T>
class Asset {
  public:
	using type = T;
	using OnModified = ktl::delegate<>;

	Asset() = default;
	Asset(not_null<type*> t, std::string_view id, OnModified* onMod = {}) noexcept;

	operator Asset<T const>() const noexcept { return {m_t, m_uri, m_onModified}; }

	bool valid() const noexcept { return m_t != nullptr; }
	explicit operator bool() const noexcept { return valid(); }
	bool operator==(Asset<T> const& rhs) const noexcept { return m_uri == rhs.m_uri && m_t == rhs.m_t; }

	T* peek() const noexcept { return m_t; }
	T& get() const;
	T& operator*() const { return get(); }
	T* operator->() const { return &get(); }
	[[nodiscard]] OnModified::handle onModified();

	std::string_view m_uri;

  private:
	T* m_t{};
	OnModified* m_onModified{};
};

// impl

template <typename T>
Asset<T>::Asset(not_null<type*> t, std::string_view id, OnModified* onMod) noexcept : m_uri(id), m_t(t), m_onModified(onMod) {}
template <typename T>
T& Asset<T>::get() const {
	ENSURE(valid(), "Invalid asset");
	return *m_t;
}
template <typename T>
auto Asset<T>::onModified() -> OnModified::handle {
	return m_onModified ? m_onModified->make_signal() : OnModified::handle();
}
} // namespace le
