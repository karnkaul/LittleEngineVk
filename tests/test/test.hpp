#pragma once
#include <source_location>
#include <string_view>

namespace test {
class Test {
  public:
	Test();
	Test(Test const&) = default;
	Test(Test&&) = default;
	auto operator=(Test const&) -> Test& = default;
	auto operator=(Test&&) -> Test& = default;

	virtual ~Test() = default;

	[[nodiscard]] virtual auto get_name() const -> std::string_view = 0;
	virtual void run() const = 0;

  protected:
	static void do_expect(bool pred, std::string_view expr, std::source_location const& location);
	static void do_assert(bool pred, std::string_view expr, std::source_location const& location);
};
} // namespace test

#define EXPECT(pred) do_expect(pred, #pred, std::source_location::current()) // NOLINT(cppcoreguidelines-macro-usage)
#define ASSERT(pred) do_assert(pred, #pred, std::source_location::current()) // NOLINT(cppcoreguidelines-macro-usage)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ADD_TEST(Class)                                                                                                                                        \
	struct Test_##Class : ::test::Test {                                                                                                                       \
		void run() const final;                                                                                                                                \
		auto get_name() const -> std::string_view final { return #Class; }                                                                                     \
	};                                                                                                                                                         \
	inline Test_##Class const g_test_##Class{};                                                                                                                \
	inline void Test_##Class::run() const
