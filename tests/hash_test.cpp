#include <core/hash.hpp>
#include <kt/ktest/ktest.hpp>

using namespace le;

void t0(kt::executor_t const& ex) {
	io::Path const path = "some/long/path";
	std::string const str = path.generic_string();
	std::string_view const strView = str;
	Hash hPath = path;
	Hash hStr = str;
	Hash hCStr = str.c_str();
	Hash hChar = str.data();
	Hash hStrView = strView;
	Hash hLiteral = "some/long/path";
	ex.expect_eq(hPath, hStr);
	ex.expect_eq(hPath, hCStr);
	ex.expect_eq(hPath, hChar);
	ex.expect_eq(hPath, hStrView);
	ex.expect_eq(hPath, hLiteral);
}

int main() {
	return kt::runner_t({{"compare", &t0}}).run(false);
}
