#include <test/test.hpp>
#include <filesystem>
#include <format>
#include <iostream>
#include <vector>

namespace test {
namespace {
struct Assert {};

void print_failure(std::string_view type, std::string_view expr, std::source_location const& sl) {
	std::cerr << std::format("  {} failed: '{}' [{}:{}]\n", type, expr, std::filesystem::path{sl.file_name()}.filename().string(), sl.line());
}

bool g_failed{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void set_failure(std::string_view type, std::string_view expr, std::source_location const& sl) {
	print_failure(type, expr, sl);
	g_failed = true;
}

auto get_tests() -> std::vector<Test*>& {
	static auto ret = std::vector<Test*>{};
	return ret;
}
} // namespace

Test::Test() { get_tests().push_back(this); }

void Test::do_expect(bool pred, std::string_view expr, std::source_location const& location) {
	if (pred) { return; }
	set_failure("expectation", expr, location);
}

void Test::do_assert(bool pred, std::string_view expr, std::source_location const& location) {
	if (pred) { return; }
	set_failure("assertion", expr, location);
	throw Assert{};
}

auto run_test(Test& test) -> bool {
	g_failed = {};
	test.run();
	if (g_failed) {
		std::cerr << std::format("[FAILED] {}\n", test.get_name());
		return false;
	}
	std::cout << std::format("[passed] {}\n", test.get_name());
	return true;
}
} // namespace test

auto main() -> int {
	auto ret = EXIT_SUCCESS;
	for (auto* test : test::get_tests()) {
		if (!run_test(*test)) { ret = EXIT_FAILURE; }
	}
	return ret;
}
