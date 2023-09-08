#include <le/core/result.hpp>
#include <test/test.hpp>

namespace test {
namespace {
using namespace le;

constexpr auto int_v = Result<int, char>{42};
static_assert(int_v.has_value());
static_assert(int_v.value() == 42); // NOLINT

constexpr auto char_v = Result<int, char>{'x'};
static_assert(char_v.has_error());
static_assert(char_v.error() == 'x');

constexpr auto void_i = Result<int>{42};
static_assert(void_i.has_value());
static_assert(void_i.value() == 42);

constexpr auto void_v = Result<int>{};
static_assert(void_v.has_error());

auto run() -> bool {
	//
	return run_tests();
}
} // namespace
} // namespace test

auto main() -> int {
	if (!test::run()) { return EXIT_FAILURE; }
}
