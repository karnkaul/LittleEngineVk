#pragma once
#include <utility>

namespace le ::utils {
class DirtyFlag {
  public:
	class Clean {
		DirtyFlag const& m_flag;

	  public:
		constexpr Clean(DirtyFlag const& out) noexcept : m_flag(out) {}
		constexpr ~Clean() noexcept { m_flag.setDirty(false); }
		explicit constexpr operator bool() const noexcept { return m_flag.dirty(); }
	};

	constexpr DirtyFlag(bool dirty = false) noexcept : m_dirty(dirty) {}
	constexpr DirtyFlag(DirtyFlag&& rhs) noexcept : m_dirty(std::exchange(rhs.m_dirty, false)) {}
	constexpr DirtyFlag& operator=(DirtyFlag&& rhs) noexcept;

	constexpr bool dirty() const noexcept { return m_dirty; }
	constexpr void setDirty(bool dirty) const noexcept { m_dirty = dirty; }

  private:
	mutable bool m_dirty;
};

// impl

constexpr DirtyFlag& DirtyFlag::operator=(DirtyFlag&& rhs) noexcept {
	if (&rhs != this) { m_dirty = std::exchange(rhs.m_dirty, false); }
	return *this;
}
} // namespace le::utils
