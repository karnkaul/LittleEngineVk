#pragma once
#include <fmt/format.h>
#include <core/delegate.hpp>
#include <core/ecs_registry.hpp>
#include <core/time.hpp>
#include <core/transform.hpp>
#include <core/utils.hpp>
#include <engine/game/input.hpp>
#include <engine/gfx/camera.hpp>
#include <engine/gfx/light.hpp>
#include <engine/gfx/pipeline.hpp>
#include <engine/gfx/screen_rect.hpp>
#include <engine/resources/resources.hpp>
#if defined(LEVK_EDITOR)
#include <engine/game/editor_types.hpp>
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

	constexpr Prop() noexcept;
	constexpr Prop(Entity entity, Transform& transform) noexcept;

	///
	/// \brief Check whether Prop points to a valid pair of Entity/Transform
	///
	bool valid() const noexcept;

	///
	/// \brief Obtain the transform (must be valid!)
	///
	Transform const& transform() const;
	///
	/// \brief Obtain the transform (must be valid!)
	///
	Transform& transform();

	constexpr operator Entity() const noexcept;
};

///
/// \brief Return type wrapper for `spawnProp<T...>()`
///
template <typename... T>
struct TProp
{
	using type = TProp;
	using Components = Registry::Components<T&...>;

	Prop prop;
	Components components;

	constexpr operator Prop() const noexcept;
};
template <typename... T>
using TProp_t = typename TProp<T...>::type;

template <>
struct TProp<>
{
	using type = Prop;
};

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
struct TScoped final
{
	std::optional<T> t;

	constexpr TScoped() = default;
	constexpr TScoped(TScoped<T>&&) = default;
	TScoped& operator=(TScoped<T>&&);
	~TScoped();
};

struct SceneDesc final
{
	enum class Flag : s8
	{
		///
		/// \brief UI follows screen size and aspect ratio
		///
		eDynamicUI,
		///
		/// \brief UI clipped to match its aspect ratio
		///
		eScissoredUI,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	gfx::Camera defaultCam;
	Ref<gfx::Camera> camera = defaultCam;
	std::vector<gfx::DirLight> dirLights;
	stdfs::path skyboxCubemapID;
	///
	/// \brief UI Transformation Space (z is depth)
	///
	glm::vec3 uiSpace = {0.0f, 0.0f, 2.0f};
	gfx::Pipeline pipe3D;
	gfx::Pipeline pipeUI;
	glm::vec2 clearDepth = {1.0f, 0.0f};
	Colour clearColour = colours::black;
	Flags flags = Flag::eDynamicUI;
};

///
/// \brief Data structure describing game frame context
///
struct Context final
{
	Registry defaultRegistry;
	std::string name;
	gfx::ScreenRect gameRect;
	Ref<Registry> registry = defaultRegistry;

#if defined(LEVK_EDITOR)
	editor::PerFrame editorData;
#endif

	void reset();
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
/// \brief Obtain (a reference to) the scene descriptor
///
/// A SceneDesc will be added to the Registry if none exist
///
SceneDesc& sceneDesc();

///
/// \brief Obtain (a reference to) the main scene camera
///
/// A SceneDesc will be added to the Registry if none exist
///
gfx::Camera& mainCamera();

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

///
/// \brief Create a new Prop
///
/// Transform will be parented to scene root if LEVK_EDITOR is defined
///
template <typename T, typename... Args>
TProp<T> spawnProp(std::string name, Args&&... args);
///
/// \brief Create a new Prop
///
/// Transform will be parented to scene root if LEVK_EDITOR is defined
template <typename... T>
TProp_t<T...> spawnProp(std::string name);
///
/// \brief Destroy one or more props
///
void destroy(Span<Prop> props);
///
/// \brief Reset game state
///
void reset();

template <typename T>
TScoped<T>& TScoped<T>::operator=(TScoped<T>&& rhs)
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
TScoped<T>::~TScoped()
{
	if (t)
	{
		destroy(*t);
	}
}

#if defined(LEVK_EDITOR)
namespace detail
{
void setup(Prop& out_prop);
} // namespace detail
#endif

template <typename T, typename... Args>
TProp<T> spawnProp(std::string name, Args&&... args)
{
	Registry& reg = g_context.registry;
	auto ec = reg.template spawn<T, Args...>(std::move(name), std::forward<Args>(args)...);
	auto pT = reg.template attach<Transform>(ec);
	ASSERT(pT, "Invariant violated!");
	Prop prop{ec, *pT};
#if defined(LEVK_EDITOR)
	detail::setup(prop);
#endif
	return {prop, std::move(ec.components)};
}

template <typename... T>
TProp_t<T...> spawnProp(std::string name)
{
	Registry& reg = g_context.registry;
	auto ec = reg.template spawn<T...>(std::move(name));
	auto pT = reg.template attach<Transform>(ec);
	ASSERT(pT, "Invariant violated!");
	Prop prop{ec, *pT};
#if defined(LEVK_EDITOR)
	detail::setup(prop);
#endif
	if constexpr (sizeof...(T) > 0)
	{
		return {prop, std::move(ec.components)};
	}
	else
	{
		return prop;
	}
}
} // namespace gs

inline constexpr Prop::Prop() noexcept : pTransform(nullptr) {}
inline constexpr Prop::Prop(Entity entity, Transform& transform) noexcept : entity(entity), pTransform(&transform) {}
inline constexpr Prop::operator Entity() const noexcept
{
	return entity;
}

template <typename... T>
constexpr TProp<T...>::operator Prop() const noexcept
{
	return prop;
}

#if defined(LEVK_EDITOR)
namespace editor
{
template <typename Flags>
FlagsWidget<Flags>::FlagsWidget(Span<sv> ids, Flags& flags)
{
	ASSERT(ids.size() <= size, "Overflow!");
	std::size_t idx = 0;
	for (auto id : ids)
	{
		bool bVal = flags.test((type)idx);
		TWidget<bool> w(id, bVal);
		flags[(type)idx++] = bVal;
	}
}

template <typename T>
TInspector<T>::TInspector(Registry& out_registry, Entity entity, T const* pT, sv id)
	: pReg(&out_registry), entity(entity), id(id.empty() ? utils::tName<T>() : id)
{
	bNew = pT == nullptr;
	if (!bNew)
	{
		node = TreeNode(this->id);
		if (node)
		{
			bOpen = true;
			if (node.test(GUI::eRightClicked))
			{
				out_registry.detach<T>(entity);
			}
		}
	}
}

template <typename T>
TInspector<T>::TInspector(TInspector<T>&& rhs) : node(std::move(rhs.node)), pReg(rhs.pReg), id(std::move(id))
{
	bOpen = rhs.bOpen;
	bNew = rhs.bNew;
	rhs.bNew = rhs.bOpen = false;
	rhs.pReg = nullptr;
}

template <typename T>
TInspector<T>& TInspector<T>::operator=(TInspector<T>&& rhs)
{
	if (&rhs != this)
	{
		node = std::move(rhs.node);
		pReg = rhs.pReg;
		bOpen = rhs.bOpen;
		bNew = rhs.bNew;
		id = std::move(rhs.id);
		rhs.bNew = rhs.bOpen = false;
		rhs.pReg = nullptr;
	}
	return *this;
}

template <typename T>
TInspector<T>::~TInspector()
{
	if (bNew && pReg)
	{
		if (auto add = TreeNode(fmt::format("[Add {}]", id), false, true, true, false); add.test(GUI::eLeftClicked))
		{
			Registry& registry = *pReg;
			registry.attach<T>(entity);
		}
	}
}

template <typename T>
TInspector<T>::operator bool() const
{
	return bOpen;
}
} // namespace editor
#endif
} // namespace le
