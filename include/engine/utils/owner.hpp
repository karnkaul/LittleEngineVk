#pragma once
#include <algorithm>
#include <memory>
#include <vector>
#include <core/not_null.hpp>

namespace le::utils {
template <typename T, template <typename...> typename Cont = std::vector, typename... Ar>
class Owner {
  public:
	using type = T;
	using container_t = Cont<std::unique_ptr<T>, Ar...>;

	template <typename Ty>
	static constexpr bool is_derived_v = std::is_base_of_v<type, Ty>;

	template <typename Ty, typename... Args>
		requires(is_derived_v<Ty>)
	Ty& push(Args&&... args) {
		auto t = std::make_unique<Ty>(std::forward<Args>(args)...);
		auto& ret = *t;
		m_ts.insert(m_ts.end(), std::move(t));
		return ret;
	}

	template <typename Ty>
		requires(is_derived_v<std::decay_t<Ty>>)
	bool pop(Ty const* t) noexcept {
		if constexpr (std::is_same_v<container_t, std::vector<std::unique_ptr<type>, Ar...>>) {
			return std::erase_if(m_ts, [t](auto const& r) { return t == r.get(); }) > 0;
		} else {
			auto it = std::find_if(m_ts.begin(), m_ts.end(), [t](auto const& r) { return t == r.get(); });
			if (it != m_ts.end()) {
				m_ts.erase();
				return true;
			}
			return false;
		}
	}

  protected:
	container_t m_ts;
};
} // namespace le::utils
