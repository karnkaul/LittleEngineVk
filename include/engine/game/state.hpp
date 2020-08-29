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
struct Prop final
{
	Entity entity;
	Transform* pTransform;

	constexpr Prop() : pTransform(nullptr) {}
	constexpr Prop(Entity entity, Transform& transform) : entity(entity), pTransform(&transform) {}

	Transform const& transform() const;
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
using ManifestLoaded = Delegate<>;

template <typename T>
struct Scoped final
{
	std::optional<T> t;

	constexpr Scoped() = default;
	constexpr Scoped(Scoped<T>&&) = default;
	Scoped& operator=(Scoped<T>&&);
	~Scoped();
};

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

struct LoadReq final
{
	stdfs::path load;
	stdfs::path unload;
	ManifestLoaded::Callback onLoaded;
};

inline Context g_context;

[[nodiscard]] input::Token registerInput(input::Context const* pContext);

[[nodiscard]] TResult<ManifestLoaded::Token> loadManifest(LoadReq const& loadReq);
[[nodiscard]] input::Token loadInputMap(stdfs::path const& id, input::Context* out_pContext);

Prop spawnProp(std::string name);
void destroy(Span<Prop> props);
void clear();

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
