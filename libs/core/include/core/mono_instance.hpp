#pragma once
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace le {
///
/// \brief CRTP base class for modelling mono/single instances
/// Throws on duplicate instance construction
///
template <typename T>
class TMonoInstance {
  public:
	TMonoInstance(bool bActive);
	TMonoInstance(TMonoInstance&&) noexcept;
	TMonoInstance& operator=(TMonoInstance&&) noexcept;
	~TMonoInstance();

	static T* inst() noexcept;

  protected:
	inline static T* s_pInstance = nullptr;
	bool m_bActive = false;
};

// impl

template <typename T>
TMonoInstance<T>::TMonoInstance(bool bActive) : m_bActive(bActive) {
	static_assert(std::is_base_of_v<TMonoInstance<T>, T>, "T must derive from TMonoInstance");
	if (m_bActive) {
		if (s_pInstance) {
			throw std::runtime_error("Duplicate instance");
		}
		s_pInstance = static_cast<T*>(this);
	}
}
template <typename T>
TMonoInstance<T>::TMonoInstance(TMonoInstance&& rhs) noexcept : m_bActive(std::exchange(rhs.m_bActive, false)) {
}
template <typename T>
TMonoInstance<T>& TMonoInstance<T>::operator=(TMonoInstance&& rhs) noexcept {
	if (&rhs != this) {
		assert(!(m_bActive && rhs.m_bActive) && "Duplicate instance");
		if (m_bActive) {
			s_pInstance = nullptr;
		}
		m_bActive = std::exchange(rhs.m_bActive, false);
		if (m_bActive) {
			s_pInstance = static_cast<T*>(this);
		}
	}
	return *this;
}
template <typename T>
TMonoInstance<T>::~TMonoInstance() {
	if (m_bActive) {
		s_pInstance = nullptr;
	}
}
template <typename T>
T* TMonoInstance<T>::inst() noexcept {
	return s_pInstance;
}
} // namespace le
