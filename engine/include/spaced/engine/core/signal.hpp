#pragma once
#include <spaced/engine/core/ptr.hpp>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace spaced {
template <typename... Args>
class Signal {
  public:
	using Callback = std::function<void(Args const&...)>;

	class Listener;

	class Handle;

	Signal(Signal&&) noexcept = default;
	auto operator=(Signal&&) noexcept -> Signal& = default;

	Signal(Signal const&) = delete;
	auto operator=(Signal const&) -> Signal& = delete;

	Signal() = default;
	~Signal() = default;

	auto connect(Callback callback) -> Handle {
		if (!m_storage) { return {}; }
		auto id_ = ++m_prev_id;
		m_storage->push_back({std::move(callback), id_});
		return Handle{m_storage, id_};
	}

	template <typename Type, typename Func>
	auto connect(Ptr<Type> object, Func member_func) -> Handle {
		return connect([object, member_func](Args const&... args) { (object->*member_func)(args...); });
	}

	auto dispatch(Args const&... args) const -> void {
		if (!m_storage) { return; }
		auto storage = *m_storage;
		for (auto const& entry : storage) { entry.callback(args...); }
	}

	auto operator()(Args const&... args) const -> void { dispatch(args...); }

	[[nodiscard]] auto empty() const -> bool { return m_storage->empty(); }

  private:
	struct Entry {
		Callback callback{};
		std::uint64_t id{};
	};

	using Storage = std::vector<Entry>;

	std::shared_ptr<Storage> m_storage{std::make_shared<Storage>()};
	std::uint64_t m_prev_id{};
};

template <typename... Args>
class Signal<Args...>::Handle {
  public:
	Handle() = default;

	[[nodiscard]] auto active() const -> bool {
		auto storage = m_storage.lock();
		if (!storage) { return false; }
		auto const func = [id_ = m_id](Entry const& entry) { return entry.id == id_; };
		return std::find_if(storage->begin(), storage->end(), func) != storage->end();
	}

	auto disconnect() -> void {
		if (m_id == 0) { return; }
		auto storage = m_storage.lock();
		if (!storage) { return; }
		std::erase_if(*storage, [id_ = m_id](Entry const& entry) { return entry.id == id_; });
		m_id = {};
	}

	explicit operator bool() const { return active(); }

  private:
	Handle(std::shared_ptr<Storage> const& storage, std::uint64_t id_) : m_storage(storage), m_id(id_) {}

	std::weak_ptr<Storage> m_storage{};
	std::uint64_t m_id{};

	friend class Signal;
};

template <typename... Args>
class Signal<Args...>::Listener {
  public:
	Listener() = default;

	Listener(Listener&&) noexcept = default;
	auto operator=(Listener&& rhs) noexcept -> Listener& {
		if (&rhs != this) {
			disconnect();
			m_handle = std::move(rhs.m_handle);
		}
		return *this;
	}

	auto operator=(Listener const&) -> Listener& = delete;
	Listener(Listener const&) = delete;

	Listener(Handle handle) : m_handle(std::move(handle)) {}
	~Listener() { disconnect(); }

	auto handle() const -> Handle const& { return m_handle; }
	void disconnect() { m_handle.disconnect(); }

	[[nodiscard]] auto connected() const -> bool { return static_cast<bool>(m_handle); }

	explicit operator bool() const { return connected(); }

  private:
	Handle m_handle{};
};
} // namespace spaced
