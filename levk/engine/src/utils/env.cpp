#include <core/io/fs_media.hpp>
#include <core/os.hpp>
#include <core/span.hpp>
#include <core/utils/data_store.hpp>
#include <core/utils/string.hpp>
#include <graphics/context/physical_device.hpp>
#include <graphics/render/surface.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/utils/env.hpp>
#include <iostream>
#include <sstream>

namespace le {
namespace {
struct Parser : clap::option_parser {
	enum Flag { eVSync };

	Parser() {
		static constexpr clap::option opts[] = {
			{eVSync, "vSync", "override vSync", "VSYNC"},
			{'v', "validation", "force Vulkan validation layers on/off", "VALIDN", clap::option::flag_optional},
			{'t', "test", "quote test", "ARG"},
		};
		spec.options = opts;
		spec.doc_desc = "VSYNC\t: off, on, adaptive, triple-buffer/triple"
						"\nVALIDN\t: off (default), on";
	}

	bool operator()(clap::option_key key, clap::str_t arg, clap::parse_state) override {
		switch (key) {
		case eVSync: {
			graphics::VSync vSync;
			if (arg == "off") {
				vSync = graphics::VSync::eOff;
			} else if (arg == "on") {
				vSync = graphics::VSync::eOn;
			} else if (arg == "adaptive") {
				vSync = graphics::VSync::eAdaptive;
			} else {
				return false;
			}
			DataStore::set("vSync", vSync);
			std::cout << "Overriding VSYNC to " << graphics::vSyncNames[vSync] << "\n";
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
					auto const& devices = Engine::availableDevices();
					if (idx < devices.size()) {
						auto const& device = devices[std::size_t(i)];
						DataObject<Engine::CustomDevice>("gpuOverride")->name = std::string(device.name());
						std::cout << "GPU Override set to [" << device.name() << "]\n";
					} else {
						std::cerr << "Invalid GPU Override: " << arg << "; total: " << devices.size() << '\n';
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
	os::environment(Span(argv, static_cast<std::size_t>(argc)));
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
	auto data = io::FSMedia::findUpwards(os::environment().paths.bin(), pattern, maxHeight);
	if (!data) { return std::nullopt; }
	return std::move(data).value();
}
} // namespace le
