#include <cstring>
#include <memory>
#include <string>
#include <core/maths.hpp>
#include <core/std_types.hpp>
#include <ktest/ktest.hpp>
#include <ktl/fixed_any.hpp>

namespace {
using namespace le;

struct vec2 final {
	f32 x = 0;
	f32 y = 0;
};

bool operator==(vec2 const& l, vec2 const& r) noexcept { return maths::equals(l.x, r.x) && maths::equals(l.y, r.y); }

template <typename T, std::size_t N>
bool compare(std::vector<T> const& lhs, ktl::fixed_any<N> const& any) {
	if (any.template contains<std::vector<T>>()) {
		auto const& rhs = any.template get<std::vector<T>>();
		if (lhs.size() == rhs.size()) { return std::equal(lhs.begin(), lhs.end(), rhs.begin()); }
	}
	return false;
}

TEST(kt_fixed_any_t_all) {
	char const* szHello = "hello";
	ktl::fixed_any<> any = szHello;
	auto szTest = any.get<char const*>();
	ASSERT_NE(szTest, nullptr);
	EXPECT_EQ(std::strcmp(szTest, szHello), 0);
	auto any1 = std::move(any);
	any.clear();
	EXPECT_EQ(any.empty(), true);
	ASSERT_EQ(any1.contains<char const*>(), true);
	EXPECT_EQ(std::strcmp(any1.get<char const*>(), szHello), 0);
	std::vector<vec2> foos(2, vec2());
	ktl::fixed_any<64> any0;
	any0 = foos;
	EXPECT_EQ(compare(foos, any0), true);
	any0.clear();
	EXPECT_EQ(any0.empty(), true);
	any0 = std::move(foos);
	EXPECT_EQ(foos.empty(), true);
}
} // namespace
