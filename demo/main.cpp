#include <core/log.hpp>
#include <demo.hpp>
#include <engine/utils/env.hpp>

int main(int argc, char* argv[]) {
	using namespace le;
	if (!utils::Env::init(argc, argv, {})) {
		return 0;
	}
	auto data = utils::Env::findData("demo/data");
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
