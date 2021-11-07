#include <dens/registry.hpp>
#include <dumb_test/dtest.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace dens;

TEST(decf_invalid) {
	registry reg;
	entity blank;
	blank.registry_id = reg.id();
	EXPECT_EQ(reg.contains(blank), false);
	EXPECT_EQ(reg.attached<int>(blank), false);
}

TEST(decf_attach) {
	registry reg;
	auto e0 = reg.make_entity<int>();
	ASSERT_EQ(reg.attached<int>(e0), true);
	auto e1 = reg.make_entity();
	EXPECT_EQ(reg.contains(e1), true);
	EXPECT_EQ(reg.attached<int>(e1), false);
	reg.attach<char>(e1) = 'x';
	reg.attach<int>(e1);
	EXPECT_EQ(reg.view<int>().size(), std::size_t(2));
	EXPECT_EQ(reg.view<int>(exclude<char>()).size(), std::size_t(1));
	int* e1i = reg.find<int>(e1);
	ASSERT_NE(e1i, nullptr);
	EXPECT_EQ(*e1i, 0);
	*e1i = 42;
	e1i = &reg.attach<int>(e1);
	ASSERT_NE(e1i, nullptr);
	EXPECT_EQ(*e1i, 0);
}

TEST(decf_detach) {
	registry reg;
	using vec = std::vector<std::string>;
	reg.make_entity<int, char, vec>();
	auto e2 = reg.make_entity();
	reg.attach<char>(e2) = 'x';
	reg.attach<int>(e2);
	auto e3 = reg.make_entity<int, char>();
	reg.make_entity<char, int>();
	auto res = reg.view<int, char>().size();
	EXPECT_EQ(res, 4U);
	EXPECT_EQ(reg.view<vec>().size(), std::size_t(1));
	EXPECT_EQ(reg.view<float>().size(), std::size_t(0));
	bool resb = reg.detach<int, char>(e2);
	EXPECT_EQ(resb, true);
	res = reg.view<int, char>().size();
	EXPECT_EQ(res, 3U);
	resb = reg.detach<int, char, vec>(e3);
	EXPECT_EQ(resb, false);
	res = reg.view<int, char>().size();
	EXPECT_EQ(res, 2U);
}

TEST(decf_destroy) {
	registry reg;
	auto e0 = reg.make_entity<int, char>();
	auto e1 = reg.make_entity<int, char>();
	reg.get<int>(e1) = 42;
	EXPECT_EQ(reg.size(), 2U);
	reg.destroy(e0);
	EXPECT_EQ(reg.size(), 1U);
	EXPECT_EQ(reg.contains(e0), false);
	EXPECT_EQ(reg.contains(e1), true);
	EXPECT_EQ(reg.get<int>(e1), 42);
	reg.clear();
	EXPECT_EQ(reg.size(), 0U);
}
