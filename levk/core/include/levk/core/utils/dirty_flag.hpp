#pragma once
#include <levk/core/std_types.hpp>

namespace le::utils {
class DirtyFlag {
  public:
	using Flags = u8;
	static constexpr Flags all_v = 0xff;

	class Clean {
		DirtyFlag const& m_flag;
		Flags m_clean;

	  public:
		constexpr Clean(DirtyFlag const& out, Flags clean = all_v) noexcept : m_flag(out), m_clean(clean) {}
		constexpr ~Clean() noexcept { m_flag.setDirty(m_clean); }
		explicit constexpr operator bool() const noexcept { return m_flag.dirty(); }
	};

	constexpr DirtyFlag(Flags dirty = {}) noexcept : m_dirty(dirty) {}
	constexpr DirtyFlag(DirtyFlag&& rhs) noexcept { exchg(*this, rhs); }
	constexpr DirtyFlag& operator=(DirtyFlag rhs) noexcept { return (exchg(*this, rhs), *this); }

	constexpr bool dirty(Flags mask = all_v) const noexcept { return m_dirty == mask; }
	constexpr void setDirty(Flags mask = all_v) const noexcept { m_dirty = mask; }

  protected:
	static constexpr void exchg(DirtyFlag& lhs, DirtyFlag& rhs) noexcept { std::swap(lhs.m_dirty, rhs.m_dirty); }

  private:
	mutable Flags m_dirty{};
};
} // namespace le::utils
