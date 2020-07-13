#include <cstring>
#include <string>
#include <core/static_any.hpp>
#include <core/std_types.hpp>

using namespace le;

struct vec2 final
{
	f32 x = 0.0f;
	f32 y = 0.0f;
};

template <typename T>
bool copyTest(StaticAny<16> any)
{
	f32 x = any.get<f32>();
	T* pT = any.getPtr<T>();
	return std::is_same_v<T, f32> ? pT != nullptr : pT != nullptr && x == 0.0f;
}

s32 main()
{
	char const* szHello = "hello";
	StaticAny<16> any = szHello;
	if (auto szTest = any.get<char const*>())
	{
		if (std::strcmp(szTest, szHello) != 0 || !copyTest<char const*>(any))
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}
	any = 4;
	auto i = any.get<s32>();
	if (i != 4 || !copyTest<s32>(any))
	{
		return 1;
	}
	vec2 const value0 = {1.0f, -1.0f};
	vec2 const value1 = {5.0f, 5.0f};
	any = value0;
	vec2* pTest = any.getPtr<vec2>();
	if (!pTest || pTest->x != value0.x || pTest->y != value0.y || !copyTest<vec2>(any))
	{
		return 1;
	}
	any = value1;
	pTest = any.getPtr<vec2>();
	if (pTest->x != value1.x || pTest->y != value1.y || !copyTest<vec2>(any))
	{
		return 1;
	}
	return 0;
}
