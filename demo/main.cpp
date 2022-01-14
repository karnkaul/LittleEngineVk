#include <demo.hpp>
#include <levk/core/io/fs_media.hpp>
#include <levk/core/io/zip_media.hpp>
#include <levk/core/log.hpp>
#include <levk/core/utils/execute.hpp>
#include <levk/engine/utils/env.hpp>

int main(int argc, char const* const argv[]) {
	using namespace le;
	switch (env::init(argc, argv)) {
	case clap::parse_result::quit: return 0;
	case clap::parse_result::parse_error: return 10;
	default: break;
	}
	// {
	// 	auto data = env::findData("demo/data.zip");
	// 	if (data) {
	// 		io::FSMedia fr;
	// 		if (auto zip = fr.bytes(*data)) {
	// 			io::ZIPMedia zr;
	// 			if (zr.mount("demo/data.zip", std::move(*zip))) {
	// 				return utils::Execute([&zr]() { return demo::run(zr) ? 0 : 10; });
	// 			}
	// 		}
	// 	}
	// }
	auto data = env::findData("demo/data");
	if (!data) {
		logE("FATAL: Failed to locate data!");
		return 1;
	}
	io::FSMedia media;
	media.mount(std::move(*data));
	return utils::Execute([&media]() { return demo::run(media) ? 0 : 10; });
}
