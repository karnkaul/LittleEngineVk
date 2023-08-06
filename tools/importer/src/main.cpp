#include <clap/clap.hpp>
#include <le/error.hpp>
#include <le/importer/importer.hpp>
#include <format>
#include <iostream>

auto main(int argc, char** argv) -> int {
	auto options = clap::Options{clap::make_app_name(*argv), "littl-engine mesh importer", LE_IMPORTER_VERSION};

	auto input = le::importer::Input{};
	auto meshes = std::vector<std::size_t>{};
	auto list = bool{};

	options.positional(input.gltf_path, "gltf-path", "gltf-path")
		.required(input.data_root, "d,data-root", "data root to export to", ".")
		.required(input.uri_prefix, "p,prefix", "URI prefix for exported assets", "rel-path")
		.unmatched(meshes, "[mesh]")
		.flag(list, "l,list", "list (exportable) assets")
		.flag(input.force, "f,force", "force export (remove existing assets)")
		.flag(input.verbose, "v,verbose", "verbose mode");

	auto const result = options.parse(argc, argv);

	if (clap::should_quit(result)) { return clap::return_code(result); }

	auto importer = le::importer::Importer{};
	try {
		auto mesh_id = std::size_t{};
		if (!meshes.empty()) {
			if (meshes.size() > 1) {
				std::cerr << " importing more than one mesh is not supported\n";
				return EXIT_FAILURE;
			}
			mesh_id = meshes.front();
		}

		if (!importer.setup(std::move(input))) { return EXIT_FAILURE; }

		if (list) {
			importer.print_assets();
			return EXIT_SUCCESS;
		}

		if (!importer.import_mesh(mesh_id)) { return EXIT_FAILURE; }
	} catch (le::Error const& e) {
		std::cerr << std::format("{}\n", e.what());
		return EXIT_FAILURE;
	} catch (std::exception const& e) {
		std::cerr << std::format("fatal error: {}\n", e.what());
		return EXIT_FAILURE;
	}
}
