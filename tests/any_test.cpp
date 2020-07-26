#include <cstring>
#include <string>
#include <core/maths.hpp>
#include <core/static_any.hpp>
#include <core/std_types.hpp>

using namespace le;

struct vec2 final
{
	f32 x = 0;
	f32 y = 0;
};

constexpr auto maxSize = std::max(sizeof(vec2), sizeof(char*));

bool operator==(vec2 const& l, vec2 const& r)
{
	return maths::equals(l.x, r.x) && maths::equals(l.y, r.y);
}

template <typename T, std::size_t N>
bool copyTest(StaticAny<N> any)
{
	f32 x = any.template get<f32>();
	T* pT = any.template getPtr<T>();
	bool const b0 = std::is_same_v<T, f32> ? pT != nullptr : pT != nullptr && x == 0.0f;
	auto any1 = any;
	StaticAny<N> any2 = T{};
	StaticAny<N> any3;
	bool const b1 = any.template get<T>() == any1.template get<T>() && !(any.template get<T>() == any2.template get<T>());
	bool const b2 = !(any.template get<T>() == any3.template get<T>());
	// any = std::vector<s32>();
	return b0 && b1 && b2;
}

s32 main()
{
	char const* szHello = "hello";
	StaticAny<maxSize> any = szHello;
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
	any = &value1;
	auto pVec = any.get<vec2 const*>();
	if (!pVec || !copyTest<vec2 const*>(any))
	{
		return 1;
	}
	any = nullptr;
	if (any.get<vec2 const*>())
	{
		return 1;
	}
	return 0;
}
