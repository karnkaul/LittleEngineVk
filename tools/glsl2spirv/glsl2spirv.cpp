#include <clap/clap.hpp>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <span>

namespace {
namespace fs = std::filesystem;

struct Path {
	fs::path base{};
	fs::path uri{};
};

struct Result {
	int success{};
	int failure{};
};

bool g_verbose{}; // NOLINT

struct Compiler {
	inline static std::string s_compiler{"glslc"}; // NOLINT
	inline static bool s_list{};				   // NOLINT

	fs::path dst_dir{};

	auto compile(fs::path const& glsl, Result& out) const -> void {
		auto filename = glsl.filename();
		auto const extension = filename.extension();
		if (extension != ".vert" && extension != ".frag") {
			if (g_verbose) { std::cout << std::format("-- ignoring file [{}]\n", filename.string()); }
			return;
		}

		filename += ".spv";
		auto const dest = dst_dir / filename;

		if (s_list) {
			std::cout << std::format("[{}] => [{}]\n", glsl.generic_string(), dest.generic_string());
			++out.success;
			return;
		}

		fs::create_directories(dst_dir);
		if (g_verbose) { std::cout << std::format("-- compiling GLSL [{}]\n", glsl.generic_string()); }
		auto const command = std::format("{} {} -o {}", s_compiler, glsl.string(), dest.string());
		// NOLINTNEXTLINE
		auto const result = std::system(command.c_str());
		if (result != 0) {
			std::cerr << std::format("[FAILED] [{}]\n", glsl.generic_string());
			++out.failure;
			return;
		}

		++out.success;
		std::cout << std::format("[{}] compiled successfully\n", dest.generic_string());
	}
};

struct Traverser {
	fs::path const& src_base; // NOLINT
	fs::path subdir{};

	template <typename FuncT>
	// NOLINTNEXTLINE
	auto traverse(fs::path const& src_path, FuncT&& per_file) -> void {
		if (g_verbose) { std::cout << std::format("-- traversing [{}]\n", (src_base / subdir).generic_string()); }
		if (fs::is_directory(src_path)) {
			auto child = Traverser{.src_base = src_base, .subdir = subdir / src_path.filename()};
			for (auto const& it : fs::directory_iterator{src_path}) { child.traverse(it.path(), per_file); }
		}
		if (fs::is_regular_file(src_path)) { per_file(src_path, subdir); }
	}

	template <typename FuncT>
	auto traverse(FuncT&& per_file) -> void {
		for (auto const& it : fs::directory_iterator{src_base}) { traverse(it.path(), per_file); }
	}
};

struct App {
	std::string src_dir{"."};
	fs::path dst_dir{};

	Result result{};

	[[nodiscard]] auto run() -> bool {
		auto const src_base = fs::path{src_dir};
		auto traverser = Traverser{src_base};
		auto per_file = [&](fs::path const& src, fs::path const& subdir) {
			auto const compiler = Compiler{dst_dir / subdir};
			compiler.compile(src, result);
		};

		traverser.traverse(per_file);

		if (Compiler::s_list) {
			std::cout << std::format("{} shaders to compile\n", result.success);
			return true;
		}

		auto output = std::format("{} shaders compiled\n", result.success);
		if (result.failure > 0) { std::format_to(std::back_inserter(output), "{} shaders failed to compile\n", result.failure); }
		std::cout << output;

		return result.failure == 0;
	}
};
} // namespace

auto main(int argc, char** argv) -> int {
	auto app = App{};
	auto options = clap::Options{clap::make_app_name(*argv), "GLSL to SPIR-V batch converter", "0.1"};
	auto unmatched = std::vector<std::string>{};
	options.required(Compiler::s_compiler, "c,compiler", "SPIR-V compiler to use", "glslc")
		.flag(g_verbose, "v,verbose", "verbose mode")
		.flag(Compiler::s_list, "l,list", "traverse and list source shaders")
		.unmatched(unmatched, "[src=.] [dst=src]");

	if (auto result = options.parse(argc, argv); clap::should_quit(result)) { return clap::return_code(result); }

	if (unmatched.size() > 2) {
		std::cerr << std::format("unexpected argument: '{}'\n", unmatched[2]);
		return EXIT_FAILURE;
	}

	if (!unmatched.empty()) {
		app.src_dir = unmatched[0];
		if (unmatched.size() > 1) {
			app.dst_dir = unmatched[1];
		} else {
			app.dst_dir = app.src_dir;
		}
	}

	try {
		if (!app.run()) { return EXIT_FAILURE; }
	} catch (std::exception const& err) {
		std::cerr << std::format("\nfatal error: {}\n", err.what());
		return EXIT_FAILURE;
	}
}
