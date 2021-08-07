#include <core/log.hpp>
#include <core/utils/execute.hpp>
#include <demo.hpp>
#include <engine/utils/env.hpp>

int main(int argc, char const* const argv[]) {
	using namespace le;
	if (env::init(argc, argv, {}) == env::Run::quit) { return 0; }
	auto data = env::findData("demo/data");
	if (!data) {
		logE("FATAL: Failed to locate data!");
		return 1;
	}
	io::FileReader reader;
	reader.mount(std::move(*data));
	return utils::Execute([&reader]() { return demo::run(reader) ? 0 : 1; });
}
