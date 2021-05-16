#include <cstring>
#include <memory>
#include <string>
#include <core/maths.hpp>
#include <core/std_types.hpp>
#include <kt/fixed_any/fixed_any.hpp>
#include <kt/ktest/ktest.hpp>

using namespace le;

struct vec2 final {
	f32 x = 0;
	f32 y = 0;
};

bool operator==(vec2 const& l, vec2 const& r) { return maths::equals(l.x, r.x) && maths::equals(l.y, r.y); }

template <typename T, std::size_t N>
bool compare(std::vector<T> const& lhs, kt::fixed_any<N> const& any) {
	if (any.template contains<std::vector<T>>()) {
		auto const& rhs = any.template get<std::vector<T>>();
		if (lhs.size() == rhs.size()) { return std::equal(lhs.begin(), lhs.end(), rhs.begin()); }
	}
	return false;
}

void t0(kt::executor_t const& ex) {
	char const* szHello = "hello";
	kt::fixed_any<> any = szHello;
	auto szTest = any.get<char const*>();
	ex.assert_neq(szTest, nullptr);
	ex.expect_eq(std::strcmp(szTest, szHello), 0);
	auto any1 = std::move(any);
	any.clear();
	ex.expect_eq(any.empty(), true);
	ex.assert_eq(any1.contains<char const*>(), true);
	ex.expect_eq(std::strcmp(any1.get<char const*>(), szHello), 0);
	std::vector<vec2> foos(2, vec2());
	kt::fixed_any<64> any0;
	any0 = foos;
	ex.expect_eq(compare(foos, any0), true);
	any0.clear();
	ex.expect_eq(any0.empty(), true);
	any0 = std::move(foos);
	ex.expect_eq(foos.empty(), true);
}

int main() {
	kt::test_map_t tests;
	tests["kt::fixed_any_t-All"] = &t0;
	return kt::runner_t(std::move(tests)).run(false);
}
