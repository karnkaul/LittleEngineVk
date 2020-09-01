#pragma once
#include <core/delegate.hpp>
#include <core/ecs_registry.hpp>
#include <core/time.hpp>
#include <core/transform.hpp>
#include <core/utils.hpp>
#include <engine/game/input.hpp>
#include <engine/gfx/camera.hpp>
#include <engine/gfx/screen_rect.hpp>

#if defined(LEVK_EDITOR)
#include <unordered_set>
#endif

namespace le
{
///
/// \brief Wrapper for Entity + Transform
///
struct Prop final
{
	Entity entity;
	Transform* pTransform;

	constexpr Prop() : pTransform(nullptr) {}
	constexpr Prop(Entity entity, Transform& transform) : entity(entity), pTransform(&transform) {}

	///
	/// \brief Check whether Prop points to a valid pair of Entity/Transform
	///
	bool valid() const;

	///
	/// \brief Obtain the transform (must be valid!)
	///
	Transform const& transform() const;
	///
	/// \brief Obtain the transform (must be valid!)
	///
	Transform& transform();

	constexpr operator Entity const()
	{
		return entity;
	}
};

#if defined(LEVK_EDITOR)
namespace editor
{
struct Dropdown final
{
	std::string title;
	std::string_view preSelect;
	std::unordered_set<std::string_view> entries;
	std::function<void(std::string_view)> onSelect;
};

struct PerFrame
{
	std::vector<editor::Dropdown> dropdowns;
};
} // namespace editor
#endif

namespace gs
{
///
/// \brief Typedef for callback on manifest loaded
///
using ManifestLoaded = Delegate<>;

///
/// \brief RAII wrapper for Prop etc
///
template <typename T>
struct Scoped final
{
	std::optional<T> t;

	constexpr Scoped() = default;
	constexpr Scoped(Scoped<T>&&) = default;
	Scoped& operator=(Scoped<T>&&);
	~Scoped();
};

///
/// \brief Data structure describing game frame context
///
struct Context final
{
	Registry regTemp;
	std::string name;
	gfx::ScreenRect gameRect;
	Ref<Registry> registry = regTemp;
	gfx::Camera camera;

#if defined(LEVK_EDITOR)
	editor::PerFrame data;
#endif
};

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
inline Context g_context;

///
/// \brief Register input context
///
[[nodiscard]] input::Token registerInput(input::Context const* pContext);

///
/// \brief Load/unload a pair of manifests
///
[[nodiscard]] TResult<ManifestLoaded::Token> loadManifest(LoadReq const& loadReq);
///
/// \brief Load input map into input context and and register it
///
[[nodiscard]] input::Token loadInputMap(stdfs::path const& id, input::Context* out_pContext);

///
/// \brief Create a new Prop
///
/// Transform will be parented to scene root if LEVK_EDITOR is defined
///
Prop spawnProp(std::string name);
///
/// \brief Destroy one or more props
///
void destroy(Span<Prop> props);
///
/// \brief Reset game state
///
void reset();

template <typename T>
Scoped<T>& Scoped<T>::operator=(Scoped<T>&& rhs)
{
	if (&rhs != this)
	{
		if (t)
		{
			destroy(*t);
		}
		t = std::move(rhs.t);
	}
	return *this;
}

template <typename T>
Scoped<T>::~Scoped()
{
	if (t)
	{
		destroy(*t);
	}
}
} // namespace gs
} // namespace le
