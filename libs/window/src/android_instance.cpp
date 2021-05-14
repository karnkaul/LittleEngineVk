#if defined(LEVK_ANDROID)
#include <android/window.h>
#include <android_native_app_glue.h>
#include <core/log.hpp>
#include <vulkan/vulkan.hpp>
#include <window/android_instance.hpp>

namespace le::window {
namespace {
struct {
	EventQueue events;
	android_app* pApp = nullptr;
	bool bInit = false;
} g_state;

LibLogger* g_log = nullptr;

constexpr std::string_view g_name = "Window";
constexpr int windowFlags = AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON;

using lvl = dl::level;

void apollEvents(android_app* pApp) {
	if (g_state.bInit && g_state.pApp == pApp) {
		int events;
		android_poll_source* pSource;
		if (ALooper_pollAll(0, nullptr, &events, (void**)&pSource) >= 0) {
			if (pSource) {
				pSource->process(pApp, pSource);
			}
		}
		if (pApp->destroyRequested) {
			Event event;
			event.type = Event::Type::eClose;
			g_state.events.m_events.push_back(event);
		}
	}
}

void handleCmd(android_app* pApp, int32_t cmd) {
	switch (cmd) {
	case APP_CMD_INIT_WINDOW: {
		if (g_state.bInit && g_state.pApp == pApp) {
			ANativeActivity_setWindowFlags(pApp->activity, windowFlags, 0);
			g_log->log(lvl::info, 0, "[{}] Android app initialised", g_name);
			Event event;
			event.type = Event::Type::eInit;
			g_state.events.m_events.push_back(event);
		}
		break;
	}
	case APP_CMD_TERM_WINDOW: {
		if (g_state.bInit && g_state.pApp == pApp) {
			ANativeActivity_setWindowFlags(g_state.pApp->activity, 0, windowFlags);
			g_log->log(lvl::info, 0, "[{}] Android app terminated", g_name);
			Event event;
			event.type = Event::Type::eTerm;
			g_state.events.m_events.push_back(event);
			// g_state.pApp->onAppCmd = g_state.onTerm;
		}
		break;
	}
	}
}

s32 handleInput(android_app* pApp, AInputEvent* event) {
	if (g_state.bInit && g_state.pApp == pApp) {
		if ((AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)) {
			bool consumed = false;
			s32 const pointer = AMotionEvent_getAction(event);
			s32 const action = pointer & AMOTION_EVENT_ACTION_MASK;
			s32 const source = AInputEvent_getSource(event);
			std::size_t const index = std::size_t((pointer & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
			if (source == AINPUT_SOURCE_TOUCHSCREEN) {
				switch (action) {
				case AMOTION_EVENT_ACTION_POINTER_UP:
				case AMOTION_EVENT_ACTION_UP:
				case AMOTION_EVENT_ACTION_DOWN:
				case AMOTION_EVENT_ACTION_POINTER_DOWN: {
					Event ev;
					Event::Cursor cursor;
					cursor.x = (f64)AMotionEvent_getX(event, index);
					cursor.y = (f64)AMotionEvent_getY(event, index);
					cursor.id = AMotionEvent_getPointerId(event, index);
					ev.type = Event::Type::eCursor;
					ev.payload.cursor = cursor;
					g_state.events.m_events.push_back(ev);
					Event::Input input;
					input.key = Key::eMouseButton1;
					input.action = (action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_UP) ? Action::eRelease : Action::ePress;
					ev.type = Event::Type::eInput;
					ev.payload.input = input;
					g_state.events.m_events.push_back(ev);
					consumed = true;
					break;
				}
				}
			}
			return consumed ? 1 : 0;
		}
	}
	return 0;
}

void init(LibLogger& logger, android_app* pApp) {
	g_log = &logger;
	g_state.pApp = pApp;
	g_state.pApp->onAppCmd = &handleCmd;
	g_state.pApp->onInputEvent = &handleInput;
	g_state.bInit = true;
	g_log->log(lvl::info, 1, "[{}] Android Instance constructed", g_name);
}

void deinit() {
	g_state = {};
	g_log->log(lvl::info, 1, "[{}] Android Instance destroyed", g_name);
}
} // namespace

AndroidInstance::AndroidInstance(CreateInfo const& info) : IInstance(true) {
	if (g_state.bInit || g_state.pApp) {
		throw std::runtime_error("Duplicate Android instance");
	}
	if (!info.config.androidApp.contains<android_app*>()) {
		throw std::runtime_error("android_app* required to function");
	}
	init(m_log, info.config.androidApp.get<android_app*>());
	m_log.minVerbosity = info.options.verbosity;
}

AndroidInstance::~AndroidInstance() {
	if (m_bActive) {
		deinit();
	}
}

View<std::string_view> AndroidInstance::vkInstanceExtensions() const {
	static constexpr std::array ret = {"VK_KHR_surface"sv, "VK_KHR_android_surface"sv};
	return ret;
}

bool AndroidInstance::vkCreateSurface(ErasedRef vkInstance, ErasedRef out_vkSurface) const {
	if (g_state.bInit && g_state.pApp && vkInstance.contains<vk::Instance>() && out_vkSurface.contains<vk::SurfaceKHR>()) {
		auto& out_surface = out_vkSurface.get<vk::SurfaceKHR>();
		auto const& instance = vkInstance.get<vk::Instance>();
		vk::AndroidSurfaceCreateInfoKHR sinfo;
		sinfo.window = g_state.pApp->window;
		out_surface = instance.createAndroidSurfaceKHR(sinfo);
		return out_surface != vk::SurfaceKHR();
	}
	return false;
}

ErasedRef AndroidInstance::nativePtr() const noexcept { return g_state.bInit && g_state.pApp ? g_state.pApp : ErasedRef(); }

EventQueue AndroidInstance::pollEvents() {
	if (g_state.bInit && g_state.pApp) {
		apollEvents(g_state.pApp);
	}
	return std::move(g_state.events);
}
} // namespace le::window
#endif
