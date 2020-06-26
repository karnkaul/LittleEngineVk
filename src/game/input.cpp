#include <deque>
#include <unordered_set>
#include <utility>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <engine/game/input.hpp>
#include <engine/game/input_context.hpp>
#include <engine/window/window.hpp>
#include <game/input_impl.hpp>
#include <levk_impl.hpp>

namespace le
{
namespace
{
glm::vec2 g_cursorPos;
std::deque<std::pair<std::weak_ptr<s8>, input::Context const*>> g_contexts;
struct
{
	input::OnInput::Token input;
	input::OnText::Token text;
} g_tokens;

struct
{
	std::vector<std::tuple<input::Key, input::Action, input::Mods::VALUE>> keys;
	std::vector<input::Gamepad> gamepads;
	std::vector<char> text;
	std::unordered_set<input::Key> held;
	glm::vec2 mousePos;
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

glm::vec2 const& input::cursorPosition()
{
	return g_cursorPos;
}

void input::init(Window& out_mainWindow)
{
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
}

void input::fire()
{
	if (auto pWindow = engine::window(); !g_contexts.empty())
	{
		auto iter = std::remove_if(g_contexts.begin(), g_contexts.end(), [](auto const& context) { return !context.first.lock(); });
		g_contexts.erase(iter, g_contexts.end());
		g_cursorPos = pWindow->cursorPos();
		Snapshot snapshot;
		snapshot.padStates = pWindow->activeGamepads();
		snapshot.keys = std::move(g_raw.keys);
		snapshot.text = std::move(g_raw.text);
		snapshot.held.reserve(g_raw.held.size());
		for (auto c : g_raw.held)
		{
			snapshot.held.push_back(c);
		}
		size_t processed = 0;
#if defined(LEVK_DEBUG)
		static bool s_bWasConsuming = false;
		bool bConsumed = false;
#endif
		for (auto const& [token, pMapping] : g_contexts)
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
					LOG_I("[{}] [{}:{}] blocking [{}] remaining input contexts", utils::tName<Context>(), name, processed, g_contexts.size() - processed - 1);
					pPrev = pMapping;
				}
				bConsumed = true;
				if (!s_bWasConsuming)
				{
					s_bWasConsuming = bConsumed;
				}
#endif
				break;
			}
			++processed;
		}
#if defined(LEVK_DEBUG)
		if (s_bWasConsuming && !bConsumed)
		{
			LOG_I("[{}] blocking context(s) expired", utils::tName<Context>());
			s_bWasConsuming = false;
		}
#endif
	}
	return;
}

void input::deinit()
{
	g_tokens = {};
	g_raw = {};
}
} // namespace le
