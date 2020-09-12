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
using sv = std::string_view;

enum GUI
{
	eOpen,
	eLeftClicked,
	eRightClicked,
	eCOUNT_
};
using GUIState = TFlags<GUI>;

struct GUIStateful
{
	GUIState guiState;

	GUIStateful();

	void refresh();

	bool test(GUI s) const
	{
		return guiState.test(s);
	}

	virtual operator bool() const
	{
		return test(GUI::eLeftClicked);
	}
};

struct Button final : GUIStateful
{
	Button(sv id);
};

struct Combo final : GUIStateful
{
	s32 select = -1;
	sv selected;

	Combo(sv id, Span<sv> entries, sv preSelected);

	operator bool() const override
	{
		return test(GUI::eOpen);
	}
};

struct TreeNode final : GUIStateful
{
	TreeNode();
	TreeNode(sv id);
	TreeNode(sv id, bool bSelected, bool bLeaf, bool bFullWidth, bool bLeftClickOpen);
	TreeNode(TreeNode&&);
	TreeNode& operator=(TreeNode&&);
	~TreeNode();

	operator bool() const override
	{
		return test(GUI::eOpen);
	}
};

template <typename T>
struct TWidget
{
	static_assert(alwaysFalse<T>, "Invalid type");
};

template <typename T>
struct TInspector
{
	TreeNode node;
	Registry* pReg = nullptr;
	Entity entity;
	std::string id;
	bool bNew = false;
	bool bOpen = false;

	TInspector() = default;
	TInspector(Registry& out_registry, Entity entity, T const* pT, sv id = sv());
	TInspector(TInspector<T>&&);
	TInspector& operator=(TInspector<T>&&);
	~TInspector();

	operator bool() const;
};

template <>
struct TWidget<bool>
{
	TWidget(sv id, bool& out_b);
};

template <>
struct TWidget<f32>
{
	TWidget(sv id, f32& out_f);
};

template <>
struct TWidget<s32>
{
	TWidget(sv id, s32& out_b);
};

template <>
struct TWidget<std::string>
{
	TWidget(sv id, std::string& out_str, f32 width = 200.0f);
};

template <>
struct TWidget<Colour>
{
	TWidget(sv id, Colour& out_colour);
};

template <>
struct TWidget<glm::vec3>
{
	TWidget(sv id, glm::vec3& out_vec, bool bNormalised, f32 dv = 0.1f);
};

template <>
struct TWidget<glm::quat>
{
	TWidget(sv id, glm::quat& out_quat, f32 dq = 0.01f);
};

template <>
struct TWidget<Transform>
{
	TWidget(sv idPos, sv idOrn, sv idScl, Transform& out_t, f32 dPos = 0.1f, f32 dOrn = 0.0f, f32 dScl = 0.1f);
};

template <typename Enum, std::size_t N = (std::size_t)Enum::eCOUNT_>
struct TNWidget
{
	static_assert(std::is_enum_v<Enum>, "Enum must be an enum!");

	TNWidget(std::array<sv, N> const& ids, TFlags<Enum, N>& flags)
	{
		for (std::size_t idx = 0; idx < N; ++idx)
		{
			bool bVal = flags.bits[idx];
			TWidget<bool> w(ids.at(idx), bVal);
			flags.bits[idx] = bVal;
		}
	}
};

struct PerFrame
{
	std::function<void()> customRightPanel;
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
	Registry defaultRegistry;
	std::string name;
	gfx::ScreenRect gameRect;
	Ref<Registry> registry = defaultRegistry;
	gfx::Camera camera;

#if defined(LEVK_EDITOR)
	editor::PerFrame editorData;
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
/// \brief Trim leading namespaces etc from typename
///
template <typename T>
std::string guiName(T const* pT = nullptr);

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
				out_registry.destroyComponent<T>(entity);
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
			registry.addComponent<T>(entity);
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
