#pragma once
#include <string>
#include <core/colour.hpp>
#include <core/ecs_registry.hpp>
#include <core/flags.hpp>
#include <core/transform.hpp>
#include <core/utils.hpp>

#if defined(LEVK_EDITOR)
namespace le::editor
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

enum class Style
{
	eSameLine,
	eSeparator,
	eCOUNT_
};
using StyleFlags = TFlags<Style>;

struct Styler final
{
	Styler(StyleFlags flags);
};

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

template <typename Flags>
struct FlagsWidget
{
	using type = typename Flags::type;
	constexpr static std::size_t size = Flags::size;

	FlagsWidget(Span<sv> ids, Flags& out_flags);
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
	using ZeroedBuf = std::string;

	TWidget(sv id, ZeroedBuf& out_buf, f32 width = 100.0f, std::size_t max = 0);
};

template <>
struct TWidget<Colour>
{
	TWidget(sv id, Colour& out_colour);
};

template <>
struct TWidget<glm::vec2>
{
	TWidget(sv id, glm::vec2& out_vec, bool bNormalised, f32 dv = 0.1f);
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
	TWidget(sv idPos, sv idOrn, sv idScl, Transform& out_t, glm::vec3 const& dPOS = glm::vec3(0.1f, 0.01f, 0.1f));
};

struct PerFrame
{
	std::function<void()> customRightPanel;
};
} // namespace le::editor
#endif
