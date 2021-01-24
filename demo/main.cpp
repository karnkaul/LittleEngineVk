#include <engine/levk.hpp>
#include <level.hpp>

#include <demo.hpp>
#include <graphics/utils/utils.hpp>

#include <fstream>
#include <dumb_json/djson.hpp>
#include <dumb_json/error_handler.hpp>

int main(int argc, char** argv) {
	le::os::args({argc, argv});
	{
		using namespace le;
		io::FileReader reader;
		io::Path const prefix = os::dirPath(os::Dir::eWorking) / "data";
		reader.mount(prefix);
		reader.mount(os::dirPath(os::Dir::eWorking) / "demo/data");
		le::demo::CreateInfo info;
		info.args = {argc, argv};
		if (!le::demo::run(info, reader)) {
			return 1;
		}
	}

	using namespace le;

	engine::Service engine({argc, argv});
	std::array<io::Path, 2> const pathSearch = {"data", "demo/data"};
	auto dataPaths = engine::locate(pathSearch);
	engine::Info info;
	Window::Info windowInfo;
	windowInfo.config.size = {1280, 720};
	windowInfo.config.title = "LittleEngineVk Demo";
	info.windowInfo = std::move(windowInfo);
	info.dataPaths = dataPaths;
#if defined(LEVK_DEBUG)
// info.bLogVRAMallocations = true;
#endif
	if (!engine.init(std::move(info))) {
		return 1;
	}
	engine::g_shutdownSequence = engine::ShutdownSequence::eShutdown_CloseWindow;
	while (engine.running()) {
		engine.update(g_driver);
		engine.render();
	}
	g_driver.cleanup();
	return 0;
}
