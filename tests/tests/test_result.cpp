#include <le/core/result.hpp>
#include <test/test.hpp>

namespace {
using namespace le;

constexpr auto int_char_r = Result<int, char>{42};
static_assert(int_char_r.has_value());
static_assert(int_char_r.value() == 42); // NOLINT

constexpr auto int_char_e = Result<int, char>{'x'};
static_assert(int_char_e.has_error());
static_assert(int_char_e.error() == 'x');

constexpr auto int_void_r = Result<int>{42};
static_assert(int_void_r.has_value());
static_assert(int_void_r.value() == 42); // NOLINT

constexpr auto int_void_v = Result<int>{};
static_assert(int_void_v.has_error());

constexpr auto int_int_r = Result<int, int>{42};
static_assert(int_int_r.has_value());
static_assert(int_int_r.value() == 42); // NOLINT

constexpr auto int_int_e = Result<int, int>::as_error(-1);
static_assert(int_int_e.has_error());
static_assert(int_int_e.error() == -1); // NOLINT
} // namespace
