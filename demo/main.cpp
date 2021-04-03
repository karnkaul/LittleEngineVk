#include <core/log.hpp>
#include <core/os.hpp>
#include <demo.hpp>

int main(int argc, char* argv[]) {
	using namespace le;
	os::args({argc, argv});
	auto data = os::findData("demo/data");
	if (!data) {
		logE("FATAL: {}!", data.error());
		return 1;
	}
	io::FileReader reader;
	reader.mount(data.move());
	if (!le::demo::run(reader)) {
		return 1;
	}
	return 0;
}
