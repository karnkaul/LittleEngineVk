#include <deque>
#include <unordered_set>
#include <utility>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <engine/game/input.hpp>
#include <engine/game/input_context.hpp>
#include <engine/window/window.hpp>
#include <editor/editor.hpp>
#include <game/input_impl.hpp>
#include <window/window_impl.hpp>
#include <levk_impl.hpp>

namespace le
{
namespace
{
using Contexts = Tokeniser<input::Context const*>;
Contexts g_contexts;
#if defined(LEVK_EDITOR)
Contexts g_editorContexts;
#endif

WindowID g_mainWindow;

struct
{
	input::OnInput::Token input;
	input::OnText::Token text;
	input::OnMouse::Token scroll;
} g_tokens;

struct
{
	std::vector<std::tuple<input::Key, input::Action, input::Mods::VALUE>> keys;
	std::vector<input::Gamepad> gamepads;
	std::vector<char> text;
	std::unordered_set<input::Key> held;
	glm::vec2 cursorPosRaw = {};
	glm::vec2 cursorPosWorld = {};
	glm::vec2 mouseScroll = {};
} g_raw;
} // namespace

input::Token input::registerContext(Context const* pContext)
{
	return g_contexts.pushFront(pContext);
}

glm::vec2 const& input::cursorPosition(bool bRaw)
{
	return bRaw ? g_raw.cursorPosRaw : g_raw.cursorPosWorld;
}

glm::vec2 input::screenToWorld(glm::vec2 const& screen)
{
	glm::vec2 ret = screen;
	if (auto pWindow = WindowImpl::windowImpl(g_mainWindow))
	{
		auto const iSize = pWindow->windowSize();
		auto const size = glm::vec2(iSize.x, iSize.y);
		ret.x = ret.x - ((f32)size.x * 0.5f);
		ret.y = ((f32)size.y * 0.5f) - ret.y;
	}
	return ret;
}

glm::vec2 input::worldToUI(const glm::vec2& world)
{
	glm::vec2 ret = world;
	if (auto pWindow = WindowImpl::windowImpl(g_mainWindow))
	{
		auto const iSize = pWindow->framebufferSize();
		auto const size = glm::vec2(iSize.x, iSize.y);
#if defined(LEVK_EDITOR)
		auto const gameRect = editor::g_gameRect.size();
		if (gameRect.x < 1.0f || gameRect.y < 1.0f)
		{
			glm::vec2 const gameOrigin = editor::g_gameRect.midPoint();
			glm::vec2 const delta = glm::vec2(0.5f) - gameOrigin;
			ret += glm::vec2(delta.x * size.x, -delta.y * size.y);
			ret /= gameRect;
		}
#endif
		glm::vec2 const coeff = {engine::g_uiSpace.x / size.x, engine::g_uiSpace.y / size.y};
		ret *= coeff;
	}
	return ret;
}

bool input::focused()
{
	if (auto pWindow = WindowImpl::windowImpl(g_mainWindow))
	{
		return pWindow->focused();
	}
	return false;
}

void input::init(Window& out_mainWindow)
{
	g_mainWindow = out_mainWindow.id();
	g_tokens.input = out_mainWindow.registerInput([](Key key, Action action, Mods::VALUE mods) {
		g_raw.keys.push_back({key, action, mods});
		if (action == Action::ePress)
		{
			g_raw.held.insert(key);
		}
		else if (action == Action::eRelease)
		{
			g_raw.held.erase(key);
		}
	});
	g_tokens.text = out_mainWindow.registerText([](char c) { g_raw.text.push_back(c); });
	g_tokens.scroll = out_mainWindow.registerScroll([](f64 x, f64 y) { g_raw.mouseScroll += glm::vec2((f32)x, (f32)y); });
	g_bFire = true;
#if defined(LEVK_EDITOR)
	g_bEditorOnly = false;
#endif
}

#if defined(LEVK_EDITOR)
input::Token input::registerEditorContext(Context const* pContext)
{
	ASSERT(pContext, "Context is null!");
	return g_editorContexts.pushFront(pContext);
}
#endif

void input::fire()
{
	if (!g_bFire)
	{
		return;
	}
	static bool s_bWasConsuming = false;
#if defined(LEVK_EDITOR)
	static bool s_bWasConsumingEditor = false;
#endif
	Snapshot snapshot;
	snapshot.padStates = activeGamepads();
	snapshot.keys = std::move(g_raw.keys);
	snapshot.text = std::move(g_raw.text);
	snapshot.held.reserve(g_raw.held.size());
	snapshot.mouseScroll = g_raw.mouseScroll;
	g_raw.mouseScroll = {};
	for (auto c : g_raw.held)
	{
		snapshot.held.push_back(c);
	}
	auto fireContexts = [&snapshot](Contexts& contexts, [[maybe_unused]] bool& out_bWasConsuming) {
		if (auto pWindow = engine::window(); !contexts.empty())
		{
			contexts.forEach([](auto pContext) { pContext->m_bFired = false; });
			g_raw.cursorPosRaw = pWindow->cursorPos();
			if (pWindow->cursorMode() != CursorMode::eDisabled)
			{
				g_raw.cursorPosWorld = screenToWorld(g_raw.cursorPosRaw);
			}
			std::size_t processed = 0;
			bool bConsumed = false;
			contexts.forEach([&](auto pContext) {
				if (!bConsumed && pContext->consumed(snapshot))
				{
#if defined(LEVK_DEBUG)
					static Context const* pPrev = nullptr;
					if (pPrev != pContext)
					{
						static std::string_view const s_unknown = "Unknown";
						std::string_view const name = pContext->m_name.empty() ? s_unknown : pContext->m_name;
						LOG_I("[{}] [{}:{}] blocking [{}] remaining input contexts", utils::tName<Context>(), name, processed, contexts.size() - processed - 1);
						pPrev = pContext;
					}
#endif
					bConsumed = true;
#if defined(LEVK_DEBUG)
					if (!out_bWasConsuming)
					{
						out_bWasConsuming = bConsumed;
					}
#endif
					++processed;
				}
			});
#if defined(LEVK_DEBUG)
			if (out_bWasConsuming && !bConsumed)
			{
				LOG_I("[{}] blocking context(s) expired", utils::tName<Context>());
				out_bWasConsuming = false;
			}
#endif
		}
	};
#if defined(LEVK_EDITOR)
	fireContexts(g_editorContexts, s_bWasConsumingEditor);
	if (!g_bEditorOnly)
#endif
	{
		fireContexts(g_contexts, s_bWasConsuming);
	}
	return;
}

void input::deinit()
{
	g_tokens = {};
	g_raw = {};
}
} // namespace le
