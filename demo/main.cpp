#include <core/io/fs_media.hpp>
#include <core/io/zip_media.hpp>
#include <core/log.hpp>
#include <core/utils/execute.hpp>
#include <demo.hpp>
#include <engine/utils/env.hpp>

#include <dens/registry.hpp>

int main(int argc, char const* const argv[]) {
	/*dens::registry r;
	auto e = r.make_entity();
	r.attach<int>(e) = 50;
	r.attach<float>(e) = -100.0f;
	auto e1 = r.make_entity<int, float>();
	r.get<int>(e1) = 0;
	r.get<float>(e1) = 0.0f;
	auto e2 = r.make_entity<int, float>();
	r.get<int>(e2) = 42;
	r.get<float>(e2) = 3.14f;
	auto e3 = r.make_entity<char, float, int>();
	r.get<int>(e3) = -6;
	r.get<float>(e3) = 65.8f;
	auto e4 = r.make_entity<std::string, float>();
	r.get<std::string>(e4) = "a very long string to trigger heap allocs and sidestep short string optimization (SSO)";
	r.get<float>(e4) = -9.87f;
	r.attach<int>(e4, 420);
	for (auto entity : r.view<int, float>()) { le::logD("{}: int: {}, float: {}", r.name(entity), entity.get<int>(), entity.get<float>()); }
	for (auto entity : r.view<int>(dens::exclude<std::string>{})) { le::logD("{}: int: {}", r.name(entity), entity.get<int>()); }
	le::logD("{}", r.get<std::string>(e4));
	bool b = r.detach<float>(e3);
	float* f = r.find<float>(e3);
	le::logD("detached: {}, found: {}", b, f != nullptr);
	for (auto entity : r.view<int, float>()) { le::logD("{}: int: {}, float: {}", r.name(entity), entity.get<int>(), entity.get<float>()); }*/

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
