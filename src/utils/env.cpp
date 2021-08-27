#include <iostream>
#include <sstream>
#include <core/io/reader.hpp>
#include <core/os.hpp>
#include <core/span.hpp>
#include <core/utils/data_store.hpp>
#include <core/utils/string.hpp>
#include <engine/engine.hpp>
#include <engine/utils/env.hpp>
#include <engine/utils/logger.hpp>

namespace le {
namespace {
struct Parser : clap::option_parser {
	enum Flag { eVsync };

	Parser() {
		static constexpr clap::option opts[] = {
			{eVsync, "vsync", "override vsync", "VSYNC"},
			{'v', "validation", "force Vulkan validation layers on/off", "VALIDN", clap::option::flag_optional},
			{'t', "test", "quote test", "ARG"},
		};
		spec.options = opts;
		spec.doc_desc = "VSYNC\t: off, on, adaptive, triple-buffer/triple"
						"\nVALIDN\t: off (default), on";
	}

	bool operator()(clap::option_key key, clap::str_t arg, clap::parse_state) override {
		switch (key) {
		case eVsync: {
			graphics::Vsync vsync;
			if (arg == "off") {
				vsync = graphics::Vsync::eOff;
			} else if (arg == "on") {
				vsync = graphics::Vsync::eOn;
			} else if (arg == "adaptive") {
				vsync = graphics::Vsync::eAdaptive;
			} else if (arg == "triple" || arg == "triple-buffer") {
				vsync = graphics::Vsync::eTripleBuffer;
			} else {
				return false;
			}
			DataStore::set("vsync", vsync);
			std::cout << "Overriding VSYNC to " << graphics::vsyncNames[vsync] << "\n";
			return true;
		}
		case 'v': {
			graphics::Validation const vd = arg == "on" ? graphics::Validation::eOn : graphics::Validation::eOff;
			DataStore::set("validation", vd);
			std::cout << "Validation layers overriden: " << (vd == graphics::Validation::eOn ? "on" : "off") << '\n';
			return true;
		}
		case 't': {
			std::cout << "arg: " << arg << '\n';
			return true;
		}
		case no_end: return true;
		default: return false;
		}
	}
};

struct GPU : clap::option_parser {
	GPU() {
		spec.arg_doc = "GPU";
		spec.doc_desc = "GPU\t: override index";
		spec.cmd_key = "gpu";
		spec.cmd_doc = "List / override GPUs";
	}

	bool operator()(clap::option_key key, clap::str_t arg, clap::parse_state state) override {
		switch (key) {
		case no_arg: {
			if (state.arg_index() == 0) {
				// override gpu
				s64 const i = utils::toS64(arg, -1);
				if (i < 0) { return false; }
				auto const idx = std::size_t(i);
				try {
					std::size_t const total = Engine::availableDevices().size();
					if (idx < total) {
						DataObject<std::size_t>("gpuOverride") = idx;
						std::cout << "GPU Override set to: " << i << '\n';
					} else {
						std::cerr << "Invalid GPU Override: " << arg << "; total: " << total << '\n';
						return false;
					}
				} catch (std::runtime_error const& e) { std::cerr << "Failed to poll GPUs:" << e.what() << '\n'; }
				return true;
			}
			return false;
		}
		case no_end: {
			if (state.arg_index() == 0) {
				// gpu list
				try {
					std::stringstream str;
					str << "Available GPUs:\n";
					int i = 0;
					for (auto const& d : Engine::availableDevices()) { str << i++ << ". " << d << "\n"; }
					std::cout << str.str();
				} catch (std::runtime_error const& e) { std::cerr << "Failed to poll GPUs: " << e.what() << '\n'; }
				state.early_exit(true);
			}
			return true;
		}
		default: return false;
		}
	}
};
} // namespace

clap::parse_result env::init(int argc, char const* const argv[]) {
	Parser parser;
	GPU gpu;
	clap::option_parser* const cmds[] = {&gpu};
	clap::program_spec spec;
	spec.parser = &parser;
	spec.doc = "LittleEngineVk command line arguments parser";
	spec.version = Engine::version().toString(false);
	spec.cmds = cmds;
	return clap::parse_args(spec, argc, argv);
}

std::optional<io::Path> env::findData(io::Path pattern, u8 maxHeight) {
	auto data = io::FileReader::findUpwards(os::environment().paths.bin(), pattern, maxHeight);
	if (!data) { return std::nullopt; }
	return std::move(data).value();
}
} // namespace le
