#pragma once
#include <array>
#include <cstddef>
#include <stdexcept>
#include <core/os.hpp>
#include <core/traits.hpp>

namespace le {
///
/// \brief Fixed-size type erased storage
///
template <std::size_t N = sizeof(void*)>
class StaticAny final {
  public:
	///
	/// \brief Switch for throwing behaviour on type mismatches / attempts to copy uncopiable types
	///
	inline static bool s_bThrow = false;

  private:
	template <typename T>
	static constexpr bool is_different_v = !std::is_same_v<std::decay_t<T>, StaticAny<N>>;
	template <typename T>
	using is_different = require<is_different_v<T>>;

  public:
	StaticAny() noexcept = default;

	StaticAny(StaticAny&& rhs);
	StaticAny(StaticAny const& rhs);
	StaticAny& operator=(StaticAny&& rhs);
	StaticAny& operator=(StaticAny const& rhs);
	~StaticAny();

	///
	/// \brief Construct with object of type T
	///
	template <typename T, typename = is_different<T>>
	StaticAny(T&& t);
	///
	/// \brief Assign to object of type T
	///
	template <typename T, typename = is_different<T>>
	StaticAny& operator=(T&& t);
	///
	/// \brief Check if held type (if any) matches T
	///
	template <typename T>
	bool contains() const noexcept;
	///
	/// \brief Check if no type is held
	///
	bool empty() const;
	///
	/// \brief Obtain value (copy) of T
	///
	template <typename T, typename = is_different<T>>
	T val() const;
	///
	/// \brief Obtain pointer to T
	///
	template <typename T, typename = is_different<T>>
	T const* ptr() const noexcept;
	///
	/// \brief Obtain pointer to T
	///
	template <typename T, typename = is_different<T>>
	T* ptr() noexcept;
	///
	/// \brief Obtain reference to T
	/// Throws / returns static reference on type mismatch
	///
	template <typename T, typename = is_different<T>>
	T const& ref() const;
	///
	/// \brief Obtain reference to T
	/// Throws / returns static reference on type mismatch
	///
	template <typename T, typename = is_different<T>>
	T& ref();

	///
	/// \brief Destroy held type (if any)
	///
	bool clear() noexcept;

  private:
	struct Erased;

  private:
	alignas(std::max_align_t) std::array<std::byte, N> m_bytes;
	Erased const* m_pErased = nullptr;

  private:
	template <typename T>
	void construct(T&& t);

	template <typename T, typename = std::enable_if_t<!std::is_lvalue_reference_v<T>>>
	void doConstruct(T&& t);

	template <typename T>
	void doConstruct(T const& t);

  private:
	template <typename T>
	struct Tag {};
	struct Erased {
		using Move = void (*)(void* pSrc, void* pDst);
		using Copy = void (*)(void const* pSrc, void* pDst);
		using Destroy = void (*)(void const* pData);

		Move moveConstruct;
		Move moveAssign;
		Copy copyConstruct;
		Copy copyAssign;
		Destroy destroy;

		template <typename T>
		constexpr Erased(Tag<T>, typename std::enable_if_t<(std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>)>* = nullptr) noexcept
			: moveConstruct(&doMoveConstruct<T>), moveAssign(&doMoveAssign<T>), copyConstruct(&doCopyConstruct<T>), copyAssign(&doCopyAssign<T>),
			  destroy(&doDestroy<T>) {
		}

		template <typename T>
		constexpr Erased(Tag<T>, typename std::enable_if_t<!(std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>)>* = nullptr) noexcept
			: moveConstruct(&doMoveConstruct<T>), moveAssign(&doMoveAssign<T>), copyConstruct(nullptr), copyAssign(nullptr), destroy(&doDestroy<T>) {
		}

		template <typename T>
		static void doMoveConstruct(void* pSrc, void* pDst);

		template <typename T>
		static void doMoveAssign(void* pSrc, void* pDst);

		template <typename T>
		static void doCopyConstruct(void const* pSrc, void* pDst);

		template <typename T>
		static void doCopyAssign(void const* pSrc, void* pDst);

		template <typename T>
		static void doDestroy(void const* pPtr);
	};

	template <typename T>
	static Erased const* erased();
	void moveConstruct(StaticAny&& rhs);
	void copyConstruct(StaticAny const& rhs);
	StaticAny& moveAssign(StaticAny&& rhs);
	StaticAny& copyAssign(StaticAny const& rhs);
	void fail();
};

template <std::size_t N>
StaticAny<N>::StaticAny(StaticAny&& rhs) : m_pErased(rhs.m_pErased) {
	moveConstruct(std::move(rhs));
}

template <std::size_t N>
StaticAny<N>::StaticAny(StaticAny const& rhs) : m_pErased(rhs.m_pErased) {
	copyConstruct(rhs);
}

template <std::size_t N>
StaticAny<N>& StaticAny<N>::operator=(StaticAny&& rhs) {
	return moveAssign(std::move(rhs));
}

template <std::size_t N>
StaticAny<N>& StaticAny<N>::operator=(StaticAny const& rhs) {
	return copyAssign(rhs);
}

template <std::size_t N>
StaticAny<N>::~StaticAny() {
	clear();
}

template <std::size_t N>
template <typename T, typename>
StaticAny<N>::StaticAny(T&& t) {
	construct<T>(std::forward<T>(t));
}

template <std::size_t N>
template <typename T, typename>
StaticAny<N>& StaticAny<N>::operator=(T&& t) {
	construct<T>(std::forward<T>(t));
	return *this;
}

template <std::size_t N>
template <typename T>
bool StaticAny<N>::contains() const noexcept {
	return erased<T>() == m_pErased;
}

template <std::size_t N>
bool StaticAny<N>::empty() const {
	return m_pErased == nullptr;
}

template <std::size_t N>
template <typename T, typename>
T StaticAny<N>::val() const {
	if (contains<T>()) {
#if defined(LEVK_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
		return *reinterpret_cast<T const*>(m_bytes.data());
#if defined(LEVK_COMPILER_GCC)
#pragma GCC diagnostic pop
#endif
	}
	if (s_bThrow) {
		throw std::runtime_error("StaticAny: Type mismatch!");
	}
	return {};
}

template <std::size_t N>
template <typename T, typename>
T const* StaticAny<N>::ptr() const noexcept {
	if (contains<T>() || contains<T const>()) {
#if defined(LEVK_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
		return reinterpret_cast<T const*>(m_bytes.data());
#if defined(LEVK_COMPILER_GCC)
#pragma GCC diagnostic pop
#endif
	}
	return nullptr;
}

template <std::size_t N>
template <typename T, typename>
T* StaticAny<N>::ptr() noexcept {
	if (contains<T>()) {
#if defined(LEVK_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
		return reinterpret_cast<T*>(m_bytes.data());
#if defined(LEVK_COMPILER_GCC)
#pragma GCC diagnostic pop
#endif
	}
	return nullptr;
}

template <std::size_t N>
template <typename T, typename>
T const& StaticAny<N>::ref() const {
	if (contains<T>()) {
#if defined(LEVK_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
		return *reinterpret_cast<T const*>(m_bytes.data());
#if defined(LEVK_COMPILER_GCC)
#pragma GCC diagnostic pop
#endif
	}
	if (s_bThrow) {
		throw std::runtime_error("StaticAny: Type mismatch!");
	}
	static T const s_t{};
	return s_t;
}

template <std::size_t N>
template <typename T, typename>
T& StaticAny<N>::ref() {
	if (contains<T>()) {
#if defined(LEVK_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
		return *reinterpret_cast<T*>(m_bytes.data());
#if defined(LEVK_COMPILER_GCC)
#pragma GCC diagnostic pop
#endif
	}
	if (s_bThrow) {
		throw std::runtime_error("StaticAny: Type mismatch!");
	}
	static T s_t{};
	return s_t;
}

template <std::size_t N>
bool StaticAny<N>::clear() noexcept {
	if (m_pErased) {
		m_pErased->destroy(m_bytes.data());
		m_pErased = nullptr;
		return true;
	}
	return false;
}

template <std::size_t N>
template <typename T>
void StaticAny<N>::construct(T&& t) {
	if constexpr (std::is_same_v<T, std::nullptr_t>) {
		clear();
	} else {
		static_assert(is_different_v<T>, "Recursive storage forbidden!");
		static_assert(sizeof(T) <= N, "Buffer overflow!");
		doConstruct<T>(std::forward<T>(t));
	}
}

template <std::size_t N>
template <typename T, typename>
void StaticAny<N>::doConstruct(T&& t) {
	auto pErased = erased<std::decay_t<T>>();
	if (m_pErased && m_pErased == pErased) {
		m_pErased->moveAssign(std::addressof(t), m_bytes.data());
	} else {
		clear();
		m_pErased = pErased;
		m_pErased->moveConstruct(std::addressof(t), m_bytes.data());
	}
}

template <std::size_t N>
template <typename T>
void StaticAny<N>::doConstruct(T const& t) {
	static_assert(std::is_copy_assignable_v<std::decay_t<T>> && std::is_copy_constructible_v<std::decay_t<T>>, "Invalid type!");
	auto pErased = erased<std::decay_t<T>>();
	if (m_pErased && m_pErased == pErased) {
		m_pErased->copyAssign(std::addressof(t), m_bytes.data());
	} else {
		clear();
		m_pErased = pErased;
		m_pErased->copyConstruct(std::addressof(t), m_bytes.data());
	}
}

template <std::size_t N>
template <typename T>
typename StaticAny<N>::Erased const* StaticAny<N>::erased() {
	static constexpr Erased s_erased{Tag<T>()};
	return &s_erased;
}

template <std::size_t N>
void StaticAny<N>::moveConstruct(StaticAny&& rhs) {
	m_pErased->moveConstruct(&rhs, m_bytes.data());
}

template <std::size_t N>
void StaticAny<N>::copyConstruct(StaticAny const& rhs) {
	if (m_pErased) {
		if (m_pErased->copyConstruct) {
			m_pErased->copyConstruct(&rhs, m_bytes.data());
		} else {
			fail();
		}
	}
}

template <std::size_t N>
StaticAny<N>& StaticAny<N>::moveAssign(StaticAny&& rhs) {
	if (&rhs != this) {
		if (m_pErased == rhs.m_pErased) {
			if (m_pErased) {
				m_pErased->moveAssign(&rhs, m_bytes.data());
			}
		} else {
			clear();
			m_pErased = rhs.m_pErased;
			if (m_pErased) {
				m_pErased->moveConstruct(&rhs, m_bytes.data());
			}
		}
	}
	return *this;
}

template <std::size_t N>
StaticAny<N>& StaticAny<N>::copyAssign(StaticAny const& rhs) {
	if (rhs.m_pErased && (!rhs.m_pErased->copyAssign || !rhs.m_pErased->copyConstruct)) {
		clear();
		fail();
	} else if (&rhs != this) {
		if (m_pErased == rhs.m_pErased) {
			if (m_pErased) {
				m_pErased->copyAssign(&rhs, m_bytes.data());
			}
		} else {
			clear();
			m_pErased = rhs.m_pErased;
			if (m_pErased) {
				m_pErased->copyConstruct(&rhs, m_bytes.data());
			}
		}
	}
	return *this;
}

template <std::size_t N>
void StaticAny<N>::fail() {
	m_pErased = nullptr;
	if (s_bThrow) {
		throw std::runtime_error("StaticAny: Attempt to copy uncopiable!");
	}
}

template <std::size_t N>
template <typename T>
void StaticAny<N>::Erased::doMoveConstruct(void* pSrc, void* pDst) {
	new (pDst) T(std::move(*reinterpret_cast<T*>(pSrc)));
}

template <std::size_t N>
template <typename T>
void StaticAny<N>::Erased::doMoveAssign(void* pSrc, void* pDst) {
	T* tDst = reinterpret_cast<T*>(pDst);
	*tDst = std::move(*reinterpret_cast<T*>(pSrc));
}

template <std::size_t N>
template <typename T>
void StaticAny<N>::Erased::doCopyConstruct(void const* pSrc, void* pDst) {
	new (pDst) T(*reinterpret_cast<T const*>(pSrc));
}

template <std::size_t N>
template <typename T>
void StaticAny<N>::Erased::doCopyAssign(void const* pSrc, void* pDst) {
	T* tDst = reinterpret_cast<T*>(pDst);
	*tDst = *reinterpret_cast<T const*>(pSrc);
}

template <std::size_t N>
template <typename T>
void StaticAny<N>::Erased::doDestroy(void const* pPtr) {
	T const* tPtr = reinterpret_cast<T const*>(pPtr);
	tPtr->~T();
}
} // namespace le
