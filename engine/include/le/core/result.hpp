#pragma once
#include <stdexcept>
#include <variant>

namespace le {
template <typename Type>
concept NotVoidT = !std::is_void_v<Type>;

template <NotVoidT Type, typename ErrorT = void>
class Result;

template <NotVoidT Type, typename ErrorT>
	requires NotVoidT<ErrorT>
class Result<Type, ErrorT> {
  public:
	constexpr Result(Type value) : Result(std::in_place_index<i_value>, std::move(value)) {}

	constexpr Result(ErrorT error)
		requires(!std::same_as<Type, ErrorT>)
		: Result(std::in_place_index<i_error>, std::move(error)) {}

	static constexpr auto as_error(ErrorT error) -> Result { return Result{std::in_place_index<i_error>, std::move(error)}; }

	[[nodiscard]] constexpr auto has_value() const -> bool { return m_storage.index() == i_value; }
	[[nodiscard]] constexpr auto has_error() const -> bool { return m_storage.index() == i_error; }

	[[nodiscard]] constexpr auto value() const -> Type const& {
		if (!has_value()) { throw std::runtime_error{"Result does not contain a value"}; }
		return std::get<i_value>(m_storage);
	}

	[[nodiscard]] constexpr auto value() -> Type& {
		if (!has_value()) { throw std::runtime_error{"Result does not contain a value"}; }
		return std::get<i_value>(m_storage);
	}

	[[nodiscard]] constexpr auto error() const -> ErrorT const& {
		if (!has_error()) { throw std::runtime_error{"Result does not contain an error"}; }
		return std::get<i_error>(m_storage);
	}

	[[nodiscard]] constexpr auto error() -> ErrorT& {
		if (!has_error()) { throw std::runtime_error{"Result does not contain an error"}; }
		return std::get<i_error>(m_storage);
	}

	explicit constexpr operator bool() const { return has_value(); }

  private:
	static constexpr auto i_value{0};
	static constexpr auto i_error{1};

	template <typename T, typename IndexT>
	constexpr Result(IndexT index, T&& t) : m_storage(index, std::forward<T>(t)) {}

	std::variant<Type, ErrorT> m_storage{};

	template <NotVoidT T, typename E>
	friend class Result;
};

template <NotVoidT Type>
class Result<Type, void> {
  public:
	Result() = default;

	constexpr Result(Type value) : m_storage(std::in_place_index<i_value>, std::move(value)) {}

	[[nodiscard]] constexpr auto has_value() const -> bool { return m_storage.index() == i_value; }
	[[nodiscard]] constexpr auto has_error() const -> bool { return m_storage.index() == i_error; }

	[[nodiscard]] constexpr auto value() const -> Type const& {
		if (!has_value()) { throw std::runtime_error{"Result does not contain a value"}; }
		return std::get<i_value>(m_storage);
	}

	[[nodiscard]] constexpr auto value() -> Type& {
		if (!has_value()) { throw std::runtime_error{"Result does not contain a value"}; }
		return std::get<i_value>(m_storage);
	}

	explicit constexpr operator bool() const { return has_value(); }

  private:
	static constexpr auto i_value{Result<Type, int>::i_value};
	static constexpr auto i_error{Result<Type, int>::i_error};

	std::variant<Type, std::monostate> m_storage{std::monostate{}};
};
} // namespace le

// unit tests

namespace le::static_unit_test {
constexpr auto int_char_v = Result<int, char>{42};
static_assert(int_char_v.has_value());
static_assert(int_char_v.value() == 42); // NOLINT

constexpr auto int_char_e = Result<int, char>{'x'};
static_assert(int_char_e.has_error());
static_assert(int_char_e.error() == 'x');

constexpr auto int_v = Result<int>{42};
static_assert(int_v.has_value());
static_assert(int_v.value() == 42); // NOLINT

constexpr auto int_e = Result<int>{};
static_assert(int_e.has_error());

constexpr auto int_int_v = Result<int, int>{42};
static_assert(int_int_v.has_value());
static_assert(int_int_v.value() == 42); // NOLINT

constexpr auto int_int_e = Result<int, int>::as_error(-1);
static_assert(int_int_e.has_error());
static_assert(int_int_e.error() == -1); // NOLINT
} // namespace le::static_unit_test
