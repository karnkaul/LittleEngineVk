#include <engine/levk.hpp>
#include <level.hpp>

int main(int argc, char** argv)
{
	using namespace le;
	engine::Service engine(argc, argv);

	io::FileReader reader;
	engine::Info info;
	Window::Info info0;
	info0.config.size = {1280, 720};
	info0.config.title = "LittleEngineVk Demo";
	info.windowInfo = info0;
	info.pReader = &reader;
	info.dataPaths = engine.locateData({{{"data"}}, {{"demo/data"}}});
	// info.dataPaths = engine.locateData({{{"data.zip"}}, {{"demo/data.zip"}}});
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
