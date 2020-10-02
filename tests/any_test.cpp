#include <cstring>
#include <memory>
#include <string>
#include <core/maths.hpp>
#include <core/static_any.hpp>
#include <core/std_types.hpp>

using namespace le;

u32 g_inst = 0;
u32 g_destroyed = 0;

struct vec2 final {
	f32 x = 0;
	f32 y = 0;
};

constexpr auto maxSize = std::max(sizeof(vec2), sizeof(char*));

bool operator==(vec2 const& l, vec2 const& r) {
	return maths::equals(l.x, r.x) && maths::equals(l.y, r.y);
}

template <typename T, std::size_t N>
bool copyTest(StaticAny<N> any) {
	f32 x = any.template val<f32>();
	T* pT = any.template ptr<T>();
	bool const b0 = std::is_same_v<T, f32> ? pT != nullptr : pT != nullptr && x == 0.0f;
	auto any1 = any;
	StaticAny<N> any2 = T{};
	StaticAny<N> any3;
	bool const b1 = any.template val<T>() == any1.template val<T>() && !(any.template val<T>() == any2.template val<T>());
	bool const b2 = !(any.template val<T>() == any3.template val<T>());
	// any = std::vector<s32>();
	any3 = std::move(any2);
	return b0 && b1 && b2;
}

template <typename T, std::size_t N>
bool compare(std::vector<T> const& lhs, StaticAny<N> const& any) {
	if (any.template contains<std::vector<T>>()) {
		auto const& rhs = any.template ref<std::vector<T>>();
		if (lhs.size() == rhs.size()) {
			return std::equal(lhs.begin(), lhs.end(), rhs.begin());
		}
	}
	return false;
}

#define FAILIF(x)                                                                                                                                              \
	do {                                                                                                                                                       \
		if (x) {                                                                                                                                               \
			return 1;                                                                                                                                          \
		}                                                                                                                                                      \
	} while (0)

s32 main() {
	char const* szHello = "hello";
	StaticAny<maxSize> any = szHello;
	if (auto szTest = any.val<char const*>()) {
		FAILIF(std::strcmp(szTest, szHello) != 0 || !copyTest<char const*>(any));
	} else {
		return 1;
	}
	any = 4;
	auto i = any.val<s32>();
	FAILIF(i != 4 || !copyTest<s32>(any));
	vec2 const value0 = {1.0f, -1.0f};
	vec2 const value1 = {5.0f, 5.0f};
	any = value0;
	vec2* pTest = any.ptr<vec2>();
	FAILIF(!pTest || pTest->x != value0.x || pTest->y != value0.y || !copyTest<vec2>(any));
	any = value1;
	pTest = any.ptr<vec2>();
	FAILIF(pTest->x != value1.x || pTest->y != value1.y || !copyTest<vec2>(any));
	any = &value1;
	auto pVec = any.val<vec2 const*>();
	FAILIF(!pVec || !copyTest<vec2 const*>(any));
	any = nullptr;
	FAILIF(any.val<vec2 const*>());
	{
		static std::vector<vec2> s_foos(2, vec2());
		auto foos = s_foos;
		StaticAny<64> any0;
		any0 = foos;
		FAILIF(!compare(foos, any0));
		any0.clear();
		any0 = std::move(foos);
		FAILIF(!compare(s_foos, any0));
		auto uVec2 = std::make_unique<vec2>();
		auto pVec = uVec2.get();
		any0 = std::move(uVec2);
		auto any1 = any0;
		auto pVec2 = any0.ref<std::unique_ptr<vec2>>().get();
		auto bVec3 = any1.contains<std::unique_ptr<vec2>>();
		FAILIF(pVec2 != pVec || bVec3);
	}
	FAILIF(g_destroyed != g_inst);
	return 0;
}
