#pragma once
#include <fmt/format.h>
#include <core/colour.hpp>
#include <core/span.hpp>
#include <core/utils/error.hpp>
#include <core/utils/string.hpp>
#include <dens/registry.hpp>
#include <engine/editor/scene_ref.hpp>
#include <engine/scene/scene_node.hpp>
#include <graphics/render/rgba.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <ktl/move_only_function.hpp>
#include <ktl/n_tree.hpp>
#include <ktl/stack_string.hpp>
#include <optional>
#include <string>

#if defined(LEVK_EDITOR)
constexpr bool levk_editor = true;
#else
constexpr bool levk_editor = false;
#endif

namespace le {
class Editor;
namespace gui {
class TreeRoot;
}
} // namespace le

namespace le::edi {
enum GUI { eOpen, eLeftClicked, eRightClicked, eDoubleClicked, eReleased };
using GUIState = ktl::enum_flags<GUI, u8>;

enum class Style { eSameLine, eSeparator };
using StyleFlags = ktl::enum_flags<Style, u8>;

enum class WType { eInput, eDrag };

template <std::size_t N = 64>
using CStr = ktl::stack_string<N>;

f32 getWindowWidth();

struct MenuList {
	struct Menu {
		CStr<64> id;
		ktl::move_only_function<void()> callback;
		bool separator = false;
	};

	using Tree = ktl::n_tree<Menu>;

	std::vector<Tree> trees;
};

struct Styler final {
	StyleFlags flags;

	Styler(StyleFlags flags);
	Styler(glm::vec2 dummy);
	Styler(f32 sameLineX);

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

struct TreeSelect {
	struct Group {
		std::string_view id;
		std::vector<std::string_view> items;
	};
	struct Select {
		std::string_view item;
		std::string_view group;

		explicit operator bool() const noexcept { return !item.empty(); }
	};

	static Select list(Span<Group const> groups, std::string_view selected = {});
};

struct Button final : GUIStateful {
	Button(std::string_view id, std::optional<f32> hue = std::nullopt, bool small = false);
};

struct Selectable : GUIStateful {
	Selectable(std::string_view id);
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
	Pane(std::string_view id, glm::vec2 size, glm::vec2 pos, bool* open, bool blockResize = true, s32 flags = 0);
	Pane(std::string_view id, glm::vec2 size = {}, bool border = false, s32 flags = 0);
	~Pane() override;

	bool child = false;

	explicit operator bool() const override { return test(GUI::eOpen); }
};

struct Popup : GUIStateful {
	Popup(std::string_view id, bool blockResize = true, int flags = 0);
	~Popup() override;

	static void open(std::string_view id);

	explicit operator bool() const override { return test(GUI::eOpen); }
	void close();
};

struct WidgetBase {
	bool changed{};
	explicit operator bool() const noexcept { return changed; }
};

template <typename T>
struct TWidget {
	static_assert(false_v<T>, "Invalid type");
};

template <typename T>
struct TWidgetWrap {
	T out;
	bool changed = false;

	template <typename... Args>
	bool operator()(T const& t, Args&&... args) {
		out = t;
		TWidget<T> tw(std::forward<Args>(args)...);
		return t != out;
	}
};

template <typename T>
struct TInspector {
	CStr<128> id;
	std::optional<TreeNode> node;
	dens::registry* pReg = nullptr;
	dens::entity entity;
	bool bNew = false;
	bool bOpen = false;

	TInspector() = default;
	TInspector(dens::registry& out_registry, dens::entity entity, T const* pT, std::string_view id = {});
	TInspector(TInspector<T>&&);
	TInspector& operator=(TInspector<T>&&);
	~TInspector();

	explicit operator bool() const;
};

struct Payload {
	void const* data{};
	std::size_t size{};
};

struct DragDrop {
	struct Source {
		bool begun{};

		Source(int flags = 0);
		~Source();

		explicit operator bool() const noexcept { return begun; }

		template <typename T>
		void payload(std::string_view type, T const& t) const {
			payload(type, {&t, sizeof(T)});
		}

		void payload(std::string_view type, Payload payload) const;
	};

	struct Target {
		bool begun{};

		Target();
		~Target();

		Payload rawPayload(std::string_view type) const;

		explicit operator bool() const noexcept { return begun; }

		template <typename T>
		T const* payload(std::string_view type) const {
			if (auto t = rawPayload(type); t.data && t.size == sizeof(T)) { return reinterpret_cast<T const*>(t.data); }
			return {};
		}
	};
};

template <>
struct TWidget<bool> : WidgetBase {
	TWidget(std::string_view id, bool& out_b);
};

template <>
struct TWidget<f32> : WidgetBase {
	TWidget(std::string_view id, f32& out_f, f32 df = 0.1f, f32 w = 0.0f, glm::vec2 rng = {}, WType wt = WType::eDrag);
};

template <>
struct TWidget<int> : WidgetBase {
	TWidget(std::string_view id, int& out_s, f32 w = 0.0f, glm::ivec2 rng = {}, WType wt = WType::eDrag);
};

template <>
struct TWidget<char*> : WidgetBase {
	TWidget(std::string_view id, char* str, std::size_t size, f32 width = {}, int flags = {});
};

template <>
struct TWidget<std::string_view> : WidgetBase {
	TWidget(std::string_view id, std::string_view readonly, f32 width = {}, int flags = {});
};

template <>
struct TWidget<Colour> : WidgetBase {
	TWidget(std::string_view id, Colour& out_colour, bool alpha, bool input);
};

template <>
struct TWidget<graphics::RGBA> : WidgetBase {
	TWidget(std::string_view id, graphics::RGBA& out_colour, bool alpha);
};

template <>
struct TWidget<glm::vec2> : WidgetBase {
	TWidget(std::string_view id, glm::vec2& out_vec, bool bNormalised, f32 dv = 0.1f);
};

template <>
struct TWidget<glm::vec3> : WidgetBase {
	TWidget(std::string_view id, glm::vec3& out_vec, bool bNormalised, f32 dv = 0.1f);
};

template <>
struct TWidget<glm::quat> : WidgetBase {
	TWidget(std::string_view id, glm::quat& out_quat, f32 dq = 0.01f);
};

template <>
struct TWidget<Transform> : WidgetBase {
	TWidget(std::string_view idPos, std::string_view idOrn, std::string_view idScl, Transform& out_t, glm::vec3 const& dPOS = {0.1f, 0.01f, 0.1f});
};

template <>
struct TWidget<std::pair<s64, s64>> : WidgetBase {
	TWidget(std::string_view id, s64& out_t, s64 min, s64 max, s64 dt);
};

template <typename T>
TInspector<T>::TInspector(dens::registry& out_registry, dens::entity entity, T const* pT, std::string_view id)
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
		id = std::exchange(rhs.id, CStr<128>());
		pReg = std::exchange(rhs.pReg, nullptr);
		bNew = std::exchange(rhs.bNew, false);
		bOpen = std::exchange(rhs.bOpen, false);
	}
	return *this;
}

template <typename T>
TInspector<T>::~TInspector() {
	if (bNew && pReg) {
		if (auto add = TreeNode(CStr<16>("[Add %s]", id.data()), false, true, true, false); add.test(GUI::eLeftClicked)) {
			dens::registry& registry = *pReg;
			registry.attach<T>(entity);
		}
	}
}

template <typename T>
TInspector<T>::operator bool() const {
	return bOpen;
}
} // namespace le::edi
