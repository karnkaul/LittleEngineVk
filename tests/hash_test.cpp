#include <core/hash.hpp>

using namespace le;

s32 main()
{
	stdfs::path const path = "some/long/path";
	std::string const str = path.generic_string();
	std::string_view const strView = str;
	Hash hPath = path;
	Hash hStr = str;
	Hash hCStr = str.c_str();
	Hash hChar = str.data();
	Hash hStrView = strView;
	Hash hLiteral = "some/long/path";
	if (hPath == hStr && hStr == hCStr && hCStr == hChar && hChar == hStrView && hStrView == hLiteral)
	{
		return 0;
	}
	return 1;
}
