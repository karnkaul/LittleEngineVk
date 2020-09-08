#include <engine/levk.hpp>
#include <level.hpp>

int main(int argc, char** argv)
{
	using namespace le;

	engine::Service engine({argc, argv});
	auto dataPaths = engine::locate({"data", "demo/data"});
	engine::Info info;
	Window::Info windowInfo;
	windowInfo.config.size = {1280, 720};
	windowInfo.config.title = "LittleEngineVk Demo";
	info.windowInfo = std::move(windowInfo);
	info.dataPaths = dataPaths;
#if defined(LEVK_DEBUG)
	info.bLogVRAMallocations = true;
#endif
	if (!engine.init(info))
	{
		return 1;
	}
	engine::g_shutdownSequence = engine::ShutdownSequence::eShutdown_CloseWindow;
	while (engine.running())
	{
		engine.update(g_driver);
		engine.render();
	}
	g_driver.cleanup();
	return 0;
}
