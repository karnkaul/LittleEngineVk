#pragma once
#include <fmt/format.h>
#include <core/delegate.hpp>
#include <core/time.hpp>
#include <engine/game/input.hpp>
#include <engine/game/scene.hpp>
#include <engine/resources/resources.hpp>
#if defined(LEVK_EDITOR)
#include <engine/game/editor_types.hpp>
#endif

namespace le
{
namespace gs
{
///
/// \brief Typedef for callback on manifest loaded
///
using ManifestLoaded = Delegate<>;

///
/// \brief Manifest load request
///
struct LoadReq final
{
	stdfs::path load;
	stdfs::path unload;
	ManifestLoaded::Callback onLoaded;
};

///
/// \brief Global context instance
///
inline GameScene g_game;

///
/// \brief Register input context
///
[[nodiscard]] Token registerInput(input::Context const* pContext);

///
/// \brief Load/unload a pair of manifests
///
[[nodiscard]] TResult<Token> loadManifest(LoadReq const& loadReq);
///
/// \brief Load input map into input context and and register it
///
[[nodiscard]] Token loadInputMap(stdfs::path const& id, input::Context* out_pContext);

void reset();
} // namespace gs
} // namespace le
