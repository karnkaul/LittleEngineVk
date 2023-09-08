#pragma once
#include <functional>
#include <source_location>
#include <string_view>

namespace test {
struct Check {
	bool pred{};
	std::string_view expr{};

	void do_expect(std::source_location const& location = std::source_location::current()) const;
	void do_assert(std::source_location const& location = std::source_location::current()) const;
};

void add_test(std::string_view name, std::function<void()> const& func);
auto run_tests() -> bool;
} // namespace test

#define EXPECT(pred) ::test::Check{pred, #pred}.do_expect() // NOLINT(cppcoreguidelines-macro-usage)
#define ASSERT(pred) ::test::Check{pred, #pred}.do_assert() // NOLINT(cppcoreguidelines-macro-usage)

#define ADD_TEST(func) ::test::add_test(#func, func) // NOLINT(cppcoreguidelines-macro-usage)
