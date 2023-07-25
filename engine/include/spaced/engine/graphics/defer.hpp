#pragma once
#include <spaced/engine/core/mono_instance.hpp>
#include <spaced/engine/graphics/buffering.hpp>
#include <concepts>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

namespace spaced::graphics {
class DeferQueue : public MonoInstance<DeferQueue> {
  public:
	template <typename Type>
	void push(Type obj) {
		auto lock = std::scoped_lock{m_mutex};
		m_current.push_back(std::make_unique<Model<Type>>(std::move(obj)));
	}

	void next_frame();
	void clear();

  private:
	struct Base {
		virtual ~Base() = default;
	};
	template <typename T>
	struct Model : Base {
		T t;
		Model(T&& t) : t(std::move(t)) {}
	};
	using Frame = std::vector<std::unique_ptr<Base>>;

	Frame m_current{};
	std::deque<Frame> m_deferred{};
	std::mutex m_mutex{};
};

template <typename Type>
class Defer {
  public:
	Defer(Defer const&) = delete;
	auto operator=(Defer const&) -> Defer& = delete;

	Defer(Defer&&) noexcept = default;
	auto operator=(Defer&&) noexcept -> Defer& = default;

	Defer(Type obj = Type{}) : m_t(std::move(obj)) {}

	~Defer() { DeferQueue::self().push(std::move(m_t)); }

	auto get() const -> Type const& { return m_t; }
	auto get() -> Type& { return m_t; }

	operator Type const&() const { return get(); }
	operator Type&() { return get(); }

  private:
	Type m_t{};
};
} // namespace spaced::graphics
