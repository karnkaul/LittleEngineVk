#pragma once

namespace le {
///
/// \brief Wrapper type to view a T&
///
template <typename T>
class View;
///
/// \brief Wrapper type to view a (read-only) T const&
///
template <typename T>
class ConstView;
template <typename T>
using CView = ConstView<T>;

template <typename T>
class View {
  public:
	using type = T;

	constexpr View() = default;
	constexpr View(type& t) noexcept : m_pT(&t) {
	}
	constexpr type const& operator*() const {
		return *m_pT;
	}
	constexpr type const* operator->() const {
		return m_pT;
	}
	constexpr type& operator*() {
		return *m_pT;
	}
	constexpr type* operator->() {
		return m_pT;
	}
	constexpr operator type&() {
		return *m_pT;
	}
	constexpr bool valid() const noexcept {
		return m_pT != nullptr;
	}
	explicit constexpr operator bool() const noexcept {
		return valid();
	}

  protected:
	T* m_pT = nullptr;

	friend class ConstView<T>;
};

template <typename T>
class ConstView {
  public:
	using type = T;

	constexpr ConstView() = default;
	constexpr ConstView(type const& t) noexcept : m_pT(&t) {
	}
	constexpr ConstView(View<type> const& view) noexcept : m_pT(view.m_pT) {
	}
	constexpr type const& operator*() const {
		return *m_pT;
	}
	constexpr type const* operator->() const {
		return m_pT;
	}
	constexpr operator type const &() const {
		return *m_pT;
	}
	constexpr bool valid() const noexcept {
		return m_pT != nullptr;
	}
	explicit constexpr operator bool() const noexcept {
		return valid();
	}

  protected:
	T const* m_pT = nullptr;
};
} // namespace le
