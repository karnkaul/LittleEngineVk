#pragma once
#include <fmt/format.h>
#include <core/delegate.hpp>
#include <core/time.hpp>
#include <engine/game/input.hpp>
#include <engine/game/scene.hpp>
#include <engine/resources/resources.hpp>
#include <kt/result/result.hpp>

namespace le {
namespace gs {
///
/// \brief Typedef for callback on manifest loaded
///
using ManifestLoaded = Delegate<>;
///
/// \brief Typedef for returned results
///
template <typename T>
using Result = kt::result_t<T, std::string_view>;

///
/// \brief Manifest load request
///
struct LoadReq final {
	io::Path load;
	io::Path unload;
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
[[nodiscard]] Result<Token> loadManifest(LoadReq const& loadReq);
///
/// \brief Load input map into input context and and register it
///
[[nodiscard]] Token loadInputMap(io::Path const& id, input::Context* out_pContext);

void reset();
} // namespace gs
} // namespace le
