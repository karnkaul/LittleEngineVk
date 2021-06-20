#include <core/log.hpp>
#include <demo.hpp>
#include <engine/utils/env.hpp>

int main(int argc, char* argv[]) {
	using namespace le;
	if (env::init(argc, argv, {}) == env::Run::quit) { return 0; }
	auto data = env::findData("demo/data");
	if (!data) {
		logE("FATAL: {}!", data.error());
		return 1;
	}
	io::FileReader reader;
	reader.mount(std::move(data).value());
	if (!le::demo::run(reader)) { return 1; }
	return 0;
}
