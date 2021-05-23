#include <core/hash.hpp>
#include <ktest/ktest.hpp>

namespace {
using namespace le;

TEST(hash_compare) {
	io::Path const path = "some/long/path";
	std::string const str = path.generic_string();
	std::string_view const strView = str;
	Hash hPath = path;
	Hash hStr = str;
	Hash hCStr = str.c_str();
	Hash hChar = str.data();
	Hash hStrView = strView;
	Hash hLiteral = "some/long/path";
	EXPECT_EQ(hPath, hStr);
	EXPECT_EQ(hPath, hCStr);
	EXPECT_EQ(hPath, hChar);
	EXPECT_EQ(hPath, hStrView);
	EXPECT_EQ(hPath, hLiteral);
}
} // namespace
