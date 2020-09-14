#pragma once
#include <fmt/format.h>
#include <core/delegate.hpp>
#include <core/ecs_registry.hpp>
#include <core/time.hpp>
#include <core/transform.hpp>
#include <core/utils.hpp>
#include <engine/game/input.hpp>
#include <engine/gfx/camera.hpp>
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
	Registry defaultRegistry = Registry(Registry::Mode::eImmediate);
	std::string name;
	gfx::ScreenRect gameRect;
	Ref<Registry> registry = defaultRegistry;
	gfx::Camera camera;

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
/// \brief Trim leading namespaces etc from typename
///
template <typename T>
std::string guiName(T const* pT = nullptr);

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

template <typename T>
std::string guiName(T const* pT)
{
	constexpr static std::string_view prefix = "::";
	auto name = (pT ? utils::tName(*pT) : utils::tName<T>());
	auto search = name.find(prefix);
	while (search < name.size())
	{
		name = name.substr(search + prefix.size());
		search = name.find(prefix);
	}
	return name;
}
} // namespace gs

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
	: pReg(&out_registry), entity(entity), id(id.empty() ? gs::guiName<T>() : id)
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
