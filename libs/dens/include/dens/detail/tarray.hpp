#pragma once
#include <cassert>
#include <memory>
#include <unordered_map>
#include <vector>
#include <dens/detail/sign.hpp>

namespace dens::detail {
class tarray_base {
  public:
	virtual ~tarray_base() = default;

	sign_t sign() const noexcept { return m_sign; }
	bool match(sign_t s) const noexcept { return sign() == s; }

	virtual std::size_t size() const noexcept = 0;
	virtual void swap_back(std::size_t index) = 0;
	virtual void move_back(void* ptr) = 0;
	virtual void pop_back() = 0;
	virtual void pop_push_back(tarray_base* out) = 0;

  protected:
	tarray_base(sign_t s) noexcept : m_sign(s) {}

	sign_t m_sign{};
};

template <typename T>
class tarray : public tarray_base {
  public:
	tarray() noexcept : tarray_base(sign_t::make<T>()) {}

	std::size_t size() const noexcept override { return m_storage.size(); }
	void swap_back(std::size_t index) override {
		if (index + 1 < m_storage.size()) { std::swap(m_storage[index], m_storage.back()); }
	}
	void move_back(void* ptr) override {
		auto t = reinterpret_cast<T*>(ptr);
		m_storage.push_back(std::move(*t));
	}
	void pop_back() override {
		assert(!m_storage.empty());
		m_storage.pop_back();
	}
	void pop_push_back(tarray_base* out) override {
		assert(!m_storage.empty());
		if (out) {
			assert(sign() == out->sign());
			out->move_back(&m_storage.back());
		}
		m_storage.pop_back();
	}

	std::vector<T> m_storage;
};

class tarray_factory {
  public:
	template <typename T>
	sign_t register_type() {
		static auto const ret = sign_t::make<T>();
		if (!registered(ret)) {
			m_map[ret] = []() -> std::unique_ptr<tarray_base> { return std::make_unique<tarray<T>>(); };
		}
		return ret;
	}

	bool registered(sign_t sign) const noexcept { return m_map.contains(sign); }

	std::unique_ptr<tarray_base> make_tarray(sign_t type) const {
		auto it = m_map.find(type);
		assert(it != m_map.end() && it->second != nullptr);
		return it->second();
	}

  private:
	using make_tarray_t = std::unique_ptr<tarray_base> (*)();
	std::unordered_map<sign_t, make_tarray_t, sign_t::hasher> m_map;
};
} // namespace dens::detail
