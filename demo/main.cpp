#include <core/io/fs_media.hpp>
#include <core/io/zip_media.hpp>
#include <core/log.hpp>
#include <core/utils/execute.hpp>
#include <demo.hpp>
#include <engine/utils/env.hpp>

#include <dumb_ecf/registry2.hpp>

int main(int argc, char const* const argv[]) {
	decf::registry2 r;
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
	for (auto view : r.view<int, float>()) {
		for (std::size_t i = 0; i < view.size(); ++i) { le::logD("int: {}, float: {}", std::get<int&>(view[i]), std::get<float&>(view[i])); }
	}
	for (auto view : r.view<int>(decf::exclude<std::string>{})) {
		for (std::size_t i = 0; i < view.size(); ++i) { le::logD("int: {}", std::get<int&>(view[i])); }
	}
	le::logD("{}", r.get<std::string>(e4));
	bool b = r.detach<float>(e3);
	float* f = r.find<float>(e3);
	le::logD("detached: {}, found: {}", b, f != nullptr);
	for (auto view : r.view<int, float>()) {
		for (std::size_t i = 0; i < view.size(); ++i) { le::logD("int: {}, float: {}", std::get<int&>(view[i]), std::get<float&>(view[i])); }
	}

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
