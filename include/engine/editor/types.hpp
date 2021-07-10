#pragma once
#include <functional>
#include <string>
#include <fmt/format.h>
#include <core/colour.hpp>
#include <core/span.hpp>
#include <core/utils/string.hpp>
#include <dumb_ecf/registry.hpp>
#include <engine/scene/scene_node.hpp>
#include <kt/enum_flags/enum_flags.hpp>
#include <kt/n_tree/n_tree.hpp>

#if defined(LEVK_EDITOR)
constexpr bool levk_editor = true;
#else
constexpr bool levk_editor = false;
#endif

namespace le::edi {
enum GUI { eOpen, eLeftClicked, eRightClicked, eCOUNT_ };
using GUIState = kt::enum_flags<GUI>;

enum class Style { eSameLine, eSeparator, eCOUNT_ };
using StyleFlags = kt::enum_flags<Style>;

struct MenuList {
	struct Menu {
		std::string id;
		std::function<void()> callback;
		bool separator = false;
	};

	using Tree = kt::n_tree<Menu>;

	std::vector<Tree> trees;
};

struct Styler final {
	StyleFlags flags;

	Styler(StyleFlags flags);

	void operator()(std::optional<StyleFlags> flags = std::nullopt);
};

struct GUIStateful {
	GUIState guiState;

	GUIStateful();
	GUIStateful(GUIStateful&&);
	GUIStateful& operator=(GUIStateful&&);
	virtual ~GUIStateful() = default;

	void refresh();

	bool test(GUI s) const { return guiState.test(s); }

	explicit virtual operator bool() const { return test(GUI::eLeftClicked); }
};

struct Text final {
	Text(std::string_view text);
};

struct Radio {
	s32 select = -1;
	std::string_view selected;

	Radio(Span<std::string_view const> options, s32 preSelect = -1, bool sameLine = true);
};

struct Button final : GUIStateful {
	Button(std::string_view id);
};

struct Combo final : GUIStateful {
	s32 select = -1;
	std::string_view selected;

	Combo(std::string_view id, Span<std::string_view const> entries, std::string_view preSelect);

	explicit operator bool() const override { return test(GUI::eOpen); }
};

struct TreeNode final : GUIStateful {
	TreeNode(std::string_view id);
	TreeNode(std::string_view id, bool bSelected, bool bLeaf, bool bFullWidth, bool bLeftClickOpen);
	~TreeNode() override;

	explicit operator bool() const override { return test(GUI::eOpen); }
};

struct MenuBar : GUIStateful {
	struct Menu : GUIStateful {
		Menu(std::string_view id);
		~Menu() override;

		explicit operator bool() const override { return test(GUI::eOpen); }
	};

	struct Item : GUIStateful {
		Item(std::string_view id, bool separator = false);
	};

	static void walk(MenuList::Tree const& tree);

	MenuBar();
	MenuBar(MenuList const& list);
	~MenuBar();
};

struct TabBar : GUIStateful {
	struct Item : GUIStateful {
		explicit Item(std::string_view id, s32 flags = 0);
		~Item() override;

		explicit operator bool() const override { return test(GUI::eOpen); }
	};

	TabBar(std::string_view id, s32 flags = 0);
	~TabBar() override;

	explicit operator bool() const override { return test(GUI::eOpen); }
};

struct Pane : GUIStateful {
	inline static bool s_blockResize = false;

	Pane(std::string_view id, glm::vec2 size, glm::vec2 pos, bool* open, bool blockResize = true, s32 flags = 0);
	Pane(std::string_view id, glm::vec2 size = {}, bool border = false, s32 flags = 0);
	~Pane() override;

	bool child = false;

	explicit operator bool() const override { return test(GUI::eOpen); }
};

template <typename T>
struct TWidget {
	static_assert(false_v<T>, "Invalid type");
};

template <typename Flags>
struct FlagsWidget {
	using type = typename Flags::type;
	static constexpr std::size_t size = Flags::size;

	FlagsWidget(Span<std::string_view const> ids, Flags& out_flags);
};

template <typename T>
struct TWidgetWrap {
	T out;
	bool changed = false;

	template <typename... Args>
	bool operator()(T const& t, Args&&... args) {
		out = t;
		auto tw = TWidget<T>(std::forward<Args>(args)...);
		return t != out;
	}
};

template <typename T>
struct TInspector {
	std::optional<TreeNode> node;
	decf::registry* pReg = nullptr;
	decf::entity entity;
	std::string id;
	bool bNew = false;
	bool bOpen = false;

	TInspector() = default;
	TInspector(decf::registry& out_registry, decf::entity entity, T const* pT, std::string_view id = {});
	TInspector(TInspector<T>&&);
	TInspector& operator=(TInspector<T>&&);
	~TInspector();

	explicit operator bool() const;
};

template <>
struct TWidget<bool> {
	TWidget(std::string_view id, bool& out_b);
};

template <>
struct TWidget<f32> {
	TWidget(std::string_view id, f32& out_f, f32 df = 0.1f, f32 w = 0.0f, glm::vec2 lm = {});
};

template <>
struct TWidget<s32> {
	TWidget(std::string_view id, s32& out_s, f32 w = 0.0f);
};

template <>
struct TWidget<std::string> {
	using ZeroedBuf = std::string;

	TWidget(std::string_view id, ZeroedBuf& out_buf, f32 width = 100.0f, std::size_t max = 0);
};

template <>
struct TWidget<Colour> {
	TWidget(std::string_view id, Colour& out_colour);
};

template <>
struct TWidget<glm::vec2> {
	TWidget(std::string_view id, glm::vec2& out_vec, bool bNormalised, f32 dv = 0.1f);
};

template <>
struct TWidget<glm::vec3> {
	TWidget(std::string_view id, glm::vec3& out_vec, bool bNormalised, f32 dv = 0.1f);
};

template <>
struct TWidget<glm::quat> {
	TWidget(std::string_view id, glm::quat& out_quat, f32 dq = 0.01f);
};

template <>
struct TWidget<SceneNode> {
	TWidget(std::string_view idPos, std::string_view idOrn, std::string_view idScl, SceneNode& out_t, glm::vec3 const& dPOS = glm::vec3(0.1f, 0.01f, 0.1f));
};

template <>
struct TWidget<std::pair<s64, s64>> {
	TWidget(std::string_view id, s64& out_t, s64 min, s64 max, s64 dt);
};

template <typename Flags>
FlagsWidget<Flags>::FlagsWidget(Span<std::string_view const> ids, Flags& flags) {
	ensure(ids.size() <= size, "Overflow!");
	std::size_t idx = 0;
	for (auto id : ids) {
		bool bVal = flags.test((type)idx);
		TWidget<bool> w(id, bVal);
		flags[(type)idx++] = bVal;
	}
}

template <typename T>
TInspector<T>::TInspector(decf::registry& out_registry, decf::entity entity, T const* pT, std::string_view id)
	: pReg(&out_registry), entity(entity), id(id.empty() ? utils::tName<T>() : id) {
	bNew = pT == nullptr;
	if (!bNew) {
		node.emplace(this->id);
		if (*node) {
			bOpen = true;
			if (node->test(GUI::eRightClicked)) { out_registry.detach<T>(entity); }
		}
	}
}

template <typename T>
TInspector<T>::TInspector(TInspector<T>&& rhs)
	: node(std::move(rhs.node)), pReg(std::exchange(rhs.pReg, nullptr)), id(std::move(id)), bNew(std::exchange(rhs.bNew, false)),
	  bOpen(std::exchange(rhs.bOpen, false)) {}

template <typename T>
TInspector<T>& TInspector<T>::operator=(TInspector<T>&& rhs) {
	if (&rhs != this) {
		node = std::move(rhs.node);
		id = std::move(rhs.id);
		pReg = std::exchange(rhs.pReg, nullptr);
		bNew = std::exchange(rhs.bNew, false);
		bOpen = std::exchange(rhs.bOpen, false);
	}
	return *this;
}

template <typename T>
TInspector<T>::~TInspector() {
	if (bNew && pReg) {
		if (auto add = TreeNode(fmt::format("[Add {}]", id), false, true, true, false); add.test(GUI::eLeftClicked)) {
			decf::registry& registry = *pReg;
			registry.attach<T>(entity);
		}
	}
}

template <typename T>
TInspector<T>::operator bool() const {
	return bOpen;
}
} // namespace le::edi
