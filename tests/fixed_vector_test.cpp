#include <kt/fixed_vector/fixed_vector.hpp>
#include <kt/ktest/ktest.hpp>

namespace {
struct foo {
	static int total;
	static int alive;
	int inst = 0;
	int val = 0;

	foo(int val) : val(val) {
		inst = ++total;
		++alive;
	}
	foo(foo&& rhs) : val(rhs.val) {
		inst = ++total;
		++alive;
	}
	foo(foo const& rhs) : val(rhs.val) {
		inst = ++total;
		++alive;
	}
	~foo() {
		--alive;
	}
};

int foo::alive = 0;
int foo::total = 0;

void t0(kt::executor_t const& ex) {
	kt::fixed_vector<int, 4> vec0;
	ex.expect_eq(vec0.empty(), true);
	vec0 = decltype(vec0)(3, 5);
	ex.expect_eq(vec0.capacity() - vec0.size(), 1U);
	vec0.clear();
	ex.expect_eq(vec0.empty(), true);
	vec0 = {1, 2, 3, 4};
	ex.expect_eq(vec0.front(), 1);
	ex.expect_eq(vec0.back(), 4);
	vec0.pop_back();
	ex.expect_eq(vec0.back(), 3);
	ex.expect_eq(vec0[1], 2);
	auto vec1 = vec0;
	ex.expect_eq(vec0.size(), vec1.size());
	{
		std::size_t idx = 0;
		for (int& i : vec0) {
			ex.expect_eq(i, vec1.at(idx++));
		}
	}
	{
		auto const& v = vec0;
		std::size_t idx = 0;
		for (int const& i : v) {
			ex.expect_eq(i, vec1.at(idx++));
		}
	}
	vec1 = std::move(vec0);
	ex.expect_eq(vec0.empty(), true);
}

void t1(kt::executor_t const& ex) {
	kt::fixed_vector<foo, 3> vec0;
	ex.expect_eq(vec0.empty(), true);
	vec0 = decltype(vec0)(2, foo(2));
	ex.expect_eq(vec0.capacity() - vec0.size(), 1U);
	ex.expect_eq(foo::alive, 2);
	vec0.clear();
	ex.expect_eq(foo::alive, 0);
	ex.expect_eq(vec0.empty(), true);
	vec0 = {foo{5}};
	ex.expect_eq(foo::alive, 1);
	ex.expect_eq(&vec0.front(), &vec0.back());
	vec0.pop_back();
	ex.expect_eq(vec0.empty(), true);
	ex.expect_eq(foo::alive, 0);
	vec0 = {foo{1}, foo{2}};
	auto vec1 = vec0;
	ex.expect_eq(vec0.size(), vec1.size());
	ex.expect_eq(foo::alive, 4);
	{
		std::size_t idx = 0;
		for (foo& f : vec0) {
			ex.expect_eq(f.val, vec1.at(idx++).val);
		}
	}
	{
		auto const& v = vec0;
		std::size_t idx = 0;
		for (foo const& f : v) {
			ex.expect_eq(f.val, vec1.at(idx++).val);
		}
	}
	vec1 = std::move(vec0);
	ex.expect_eq(vec0.empty(), true);
	ex.expect_eq(foo::alive, 2);
	vec1.clear();
	ex.expect_eq(foo::alive, 0);
}
} // namespace

int main() {
	kt::test_map_t tests;
	tests["fixed_vector: trivial"] = &t0;
	tests["fixed_vector: class"] = &t1;
	return kt::runner_t(std::move(tests)).run(true);
}
