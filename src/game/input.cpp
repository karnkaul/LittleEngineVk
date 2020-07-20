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
using Contexts = std::deque<std::pair<std::weak_ptr<s8>, input::Context const*>>;
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
	glm::vec2 actualCursorPos = {};
	glm::vec2 virtualCursorPos = {};
	glm::vec2 mouseScroll = {};
} g_raw;
} // namespace

void input::registerContext(CtxWrapper const& context)
{
	context.token = registerContext(context.context);
}

input::Token input::registerContext(Context const& context)
{
	Token token = std::make_shared<s8>(0);
	g_contexts.emplace_front(token, &(context));
	return token;
}

glm::vec2 const& input::cursorPosition(bool bIgnoreDisabled)
{
	return bIgnoreDisabled ? g_raw.actualCursorPos : g_raw.virtualCursorPos;
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
#if defined(LEVK_EDITOR)
		auto const gameRect = editor::g_gameRect.size();
		if (gameRect.x < 1.0f || gameRect.y < 1.0f)
		{
			auto const iFbSize = pWindow->framebufferSize();
			glm::vec2 const fbSize = {(f32)iFbSize.x, (f32)iFbSize.y};
			glm::vec2 const gameOrigin = editor::g_gameRect.midPoint();
			glm::vec2 const delta = glm::vec2(0.5f) - gameOrigin;
			ret += glm::vec2(delta.x * fbSize.x, -delta.y * fbSize.y);
			ret /= gameRect;
		}
#endif
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
		glm::vec2 const coeff = {engine::g_uiSpace.x / size.x, engine::g_uiSpace.y / size.y};
		ret *= coeff;
	}
	return ret;
}

bool input::isInFocus()
{
	if (auto pWindow = WindowImpl::windowImpl(g_mainWindow))
	{
		return pWindow->isFocused();
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
void input::registerEditorContext(CtxWrapper const& context)
{
	context.token = registerEditorContext(context.context);
}

input::Token input::registerEditorContext(Context const& context)
{
	Token token = std::make_shared<s8>(0);
	g_editorContexts.emplace_front(token, &(context));
	return token;
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
			auto iter = std::remove_if(contexts.begin(), contexts.end(), [](auto const& context) { return !context.first.lock(); });
			contexts.erase(iter, contexts.end());
			for (auto& context : contexts)
			{
				context.second->m_bFired = false;
			}
			g_raw.actualCursorPos = screenToWorld(pWindow->cursorPos());
			if (pWindow->cursorMode() != CursorMode::eDisabled)
			{
				g_raw.virtualCursorPos = g_raw.actualCursorPos;
			}
			std::size_t processed = 0;
#if defined(LEVK_DEBUG)
			bool bConsumed = false;
#endif
			for (auto const& [token, pMapping] : contexts)
			{
				ASSERT(token.lock(), "Invalid token!");
				ASSERT(pMapping, "Null Context!");
#if defined(LEVK_DEBUG)

#endif
				if (token.lock() && pMapping->isConsumed(snapshot))
				{
#if defined(LEVK_DEBUG)
					static Context const* pPrev = nullptr;
					if (pPrev != pMapping)
					{
						static std::string_view const s_unknown = "Unknown";
						std::string_view const name = pMapping->m_name.empty() ? s_unknown : pMapping->m_name;
						LOG_I("[{}] [{}:{}] blocking [{}] remaining input contexts", utils::tName<Context>(), name, processed, contexts.size() - processed - 1);
						pPrev = pMapping;
					}
					bConsumed = true;
					if (!out_bWasConsuming)
					{
						out_bWasConsuming = bConsumed;
					}
#endif
					break;
				}
				++processed;
			}
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
