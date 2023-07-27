#include <clap/clap.hpp>
#include <importer/importer.hpp>
#include <spaced/engine/error.hpp>
#include <format>
#include <iostream>

auto main(int argc, char** argv) -> int {
	auto options = clap::Options{clap::make_app_name(*argv), "spaced mesh importer", SPACED_IMPORTER_VERSION};

	auto input = spaced::importer::Input{};

	options.positional(input.gltf_path, "gltf-path", "gltf-path")
		.required(input.data_root, "d,data-root", "data root to export to", ".")
		.required(input.uri_prefix, "p,prefix", "URI prefix for exported assets", "rel-path")
		.unmatched(input.meshes, "[mesh]")
		.flag(input.list, "l,list", "list (exportable) assets")
		.flag(input.force, "f,force", "force export (remove existing assets)")
		.flag(input.verbose, "v, verbose", "verbose mode");

	auto const result = options.parse(argc, argv);

	if (clap::should_quit(result)) { return clap::return_code(result); }

	try {
		return spaced::importer::Importer{}.run(std::move(input));
	} catch (spaced::Error const& e) {
		std::cerr << std::format("{}\n", e.what());
		return EXIT_FAILURE;
	} catch (std::exception const& e) {
		std::cerr << std::format("fatal error: {}\n", e.what());
		return EXIT_FAILURE;
	}
}
