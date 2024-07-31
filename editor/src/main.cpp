#include <app.hpp>
#include <print>

auto main(int argc, char** argv) -> int {
	try {
		static constexpr std::string_view assets_uri_v = "assets";
		char const* search_base_dir = argc > 0 ? *argv : ".";
		auto const assets_dir = levk::VfsFiles::upfind(assets_uri_v, search_base_dir, false);
		auto const vfs = levk::VfsFiles{assets_dir};

		auto app = App{&vfs};
		app.run(vk::SampleCountFlagBits::e2);
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	}
}
