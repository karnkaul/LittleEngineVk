#include <test/test.hpp>
#include <filesystem>
#include <format>
#include <iostream>

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

struct Test {
	std::string_view name{};
	std::function<void()> func{};
};

std::vector<Test> g_tests{}; // NOLINT
} // namespace

void Check::do_expect(std::source_location const& location) const {
	if (pred) { return; }
	set_failure("expectation", expr, location);
}

void Check::do_assert(std::source_location const& location) const {
	if (pred) { return; }
	set_failure("assertion", expr, location);
	throw Assert{};
}

auto run_test(Test const& test) -> bool {
	g_failed = {};
	test.func();
	if (g_failed) {
		std::cerr << std::format("[FAILED] {}\n", test.name);
		return false;
	}
	std::cout << std::format("[passed] {}\n", test.name);
	return true;
}
} // namespace test

void test::add_test(std::string_view name, std::function<void()> const& func) { g_tests.push_back({name, func}); }

auto test::run_tests() -> bool {
	if (g_tests.empty()) {
		std::cout << "no tests to run\n";
		return true;
	}

	auto failed = std::size_t{};
	try {
		for (auto const& test : g_tests) {
			if (!run_test(test)) { ++failed; }
		}
	} catch (std::runtime_error const& e) {
		std::cerr << std::format("fatal error: {}\n", e.what());
		return false;
	} catch (...) {
		std::cerr << "panic!\n";
		return false;
	}

	return failed == 0;
}
