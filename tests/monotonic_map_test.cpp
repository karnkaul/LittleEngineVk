#include <array>
#include <bitset>
#include <map>
#include <dumb_test/dtest.hpp>
#include <ktl/monotonic_map.hpp>

namespace {
TEST(monotonic_map_ordered) {
	using map_t = ktl::monotonic_map<int, std::map<int, int>>;
	map_t::handle mh0, mh1;
	{
		map_t map;
		mh0 = map.push(1);
		{
			mh1 = map.push(2);
			auto handle = map.push(3);
			static constexpr std::array check = {1, 2, 3};
			std::size_t idx = 0;
			for (auto const& i : map) { EXPECT_EQ(i, check[idx++]); }
			for (auto it = map.rbegin(); it != map.rend(); ++it) { EXPECT_EQ(*it, check[--idx]); }
		}
		EXPECT_EQ(map.size(), 2U);
		auto it = map.begin();
		ASSERT_NE(it, map.end());
		EXPECT_EQ(*it++, 1);
		EXPECT_EQ(*it, 2);
		EXPECT_EQ(*map.find(mh1), 2);
		mh0.reset();
		it = map.begin();
		EXPECT_EQ(*it++, 2);
		EXPECT_EQ(it, map.end());
		EXPECT_EQ(map.size(), 1U);
		{
			auto moved_map = std::move(map);
			EXPECT_EQ(map.size(), 0U);
			EXPECT_EQ(moved_map.size(), 1U);
			it = moved_map.begin();
			EXPECT_EQ(*it++, 2);
			EXPECT_EQ(it, moved_map.end());
		}
	}
}

TEST(monotonic_map_unordered) {
	using umap_t = ktl::monotonic_map<int>;
	umap_t::handle uh0, uh1;
	{
		umap_t umap;
		EXPECT_EQ(uh0.valid(), false);
		EXPECT_EQ(uh1.valid(), false);
		uh0 = umap.push(1);
		EXPECT_EQ(uh0.valid(), true);
		ASSERT_NE(umap.find(uh0), nullptr);
		EXPECT_EQ(*umap.find(uh0), 1);
		uh1 = umap.push(2);
		std::bitset<2> bits;
		for (int const& i : umap) { bits[std::size_t(i - 1)] = true; }
		EXPECT_EQ(bits.all(), true);
		uh1 = {};
		EXPECT_EQ(umap.find(uh1), nullptr);
		EXPECT_NE(umap.find(uh0), nullptr);
		EXPECT_EQ(umap.size(), 1U);
	}
	EXPECT_EQ(uh0.valid(), false);
}
} // namespace
