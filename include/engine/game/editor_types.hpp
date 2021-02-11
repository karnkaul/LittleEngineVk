#pragma once
#include <string>
#include <fmt/format.h>
#include <core/colour.hpp>
#include <core/ec_registry.hpp>
#include <core/span.hpp>
#include <core/transform.hpp>
#include <kt/enum_flags/enum_flags.hpp>

#if defined(LEVK_EDITOR)
namespace le::editor {
using sv = std::string_view;

enum GUI { eOpen, eLeftClicked, eRightClicked, eCOUNT_ };
using GUIState = kt::enum_flags<GUI>;

enum class Style { eSameLine, eSeparator, eCOUNT_ };
using StyleFlags = kt::enum_flags<Style>;

struct Styler final {
	Styler(StyleFlags flags);
};

struct GUIStateful {
	GUIState guiState;

	GUIStateful();
	GUIStateful(GUIStateful&&);
	GUIStateful& operator=(GUIStateful&&);
	virtual ~GUIStateful() = default;

	void refresh();

	bool test(GUI s) const {
		return guiState.test(s);
	}

	explicit virtual operator bool() const {
		return test(GUI::eLeftClicked);
	}
};

struct Text final {
	Text(sv text);
};

struct Button final : GUIStateful {
	Button(sv id);
};

struct Combo final : GUIStateful {
	s32 select = -1;
	sv selected;

	Combo(sv id, View<sv> entries, sv preSelected);

	explicit operator bool() const override {
		return test(GUI::eOpen);
	}
};

struct TreeNode final : GUIStateful {
	TreeNode();
	TreeNode(sv id);
	TreeNode(sv id, bool bSelected, bool bLeaf, bool bFullWidth, bool bLeftClickOpen);
	TreeNode(TreeNode&&) = default;
	TreeNode& operator=(TreeNode&&) = default;
	~TreeNode() override;

	explicit operator bool() const override {
		return test(GUI::eOpen);
	}
};

template <typename T>
struct TWidget {
	static_assert(false_v<T>, "Invalid type");
};

template <typename Flags>
struct FlagsWidget {
	using type = typename Flags::type;
	static constexpr std::size_t size = Flags::size;

	FlagsWidget(View<sv> ids, Flags& out_flags);
};

template <typename T>
struct TInspector {
	TreeNode node;
	ecs::Registry* pReg = nullptr;
	ecs::Entity entity;
	std::string id;
	bool bNew = false;
	bool bOpen = false;

	TInspector() = default;
	TInspector(ecs::Registry& out_registry, ecs::Entity entity, T const* pT, sv id = sv());
	TInspector(TInspector<T>&&);
	TInspector& operator=(TInspector<T>&&);
	~TInspector();

	explicit operator bool() const;
};

template <>
struct TWidget<bool> {
	TWidget(sv id, bool& out_b);
};

template <>
struct TWidget<f32> {
	TWidget(sv id, f32& out_f, f32 df = 0.1f, f32 w = 0.0f);
};

template <>
struct TWidget<s32> {
	TWidget(sv id, s32& out_s, f32 w = 0.0f);
};

template <>
struct TWidget<std::string> {
	using ZeroedBuf = std::string;

	TWidget(sv id, ZeroedBuf& out_buf, f32 width = 100.0f, std::size_t max = 0);
};

template <>
struct TWidget<Colour> {
	TWidget(sv id, Colour& out_colour);
};

template <>
struct TWidget<glm::vec2> {
	TWidget(sv id, glm::vec2& out_vec, bool bNormalised, f32 dv = 0.1f);
};

template <>
struct TWidget<glm::vec3> {
	TWidget(sv id, glm::vec3& out_vec, bool bNormalised, f32 dv = 0.1f);
};

template <>
struct TWidget<glm::quat> {
	TWidget(sv id, glm::quat& out_quat, f32 dq = 0.01f);
};

template <>
struct TWidget<Transform> {
	TWidget(sv idPos, sv idOrn, sv idScl, Transform& out_t, glm::vec3 const& dPOS = glm::vec3(0.1f, 0.01f, 0.1f));
};

template <>
struct TWidget<std::pair<s64, s64>> {
	TWidget(sv id, s64& out_t, s64 min, s64 max, s64 dt);
};

struct PerFrame {
	std::vector<std::function<void()>> customRightPanel;
	std::vector<std::function<void(ecs::Entity, Transform*)>> inspect;
};

template <typename Flags>
FlagsWidget<Flags>::FlagsWidget(View<sv> ids, Flags& flags) {
	ENSURE(ids.size() <= size, "Overflow!");
	std::size_t idx = 0;
	for (auto id : ids) {
		bool bVal = flags.test((type)idx);
		TWidget<bool> w(id, bVal);
		flags[(type)idx++] = bVal;
	}
}

template <typename T>
TInspector<T>::TInspector(ecs::Registry& out_registry, ecs::Entity entity, T const* pT, sv id)
	: pReg(&out_registry), entity(entity), id(id.empty() ? utils::tName<T>() : id) {
	bNew = pT == nullptr;
	if (!bNew) {
		node = TreeNode(this->id);
		if (node) {
			bOpen = true;
			if (node.test(GUI::eRightClicked)) {
				out_registry.detach<T>(entity);
			}
		}
	}
}

template <typename T>
TInspector<T>::TInspector(TInspector<T>&& rhs)
	: node(std::move(rhs.node)), pReg(std::exchange(rhs.pReg, nullptr)), id(std::move(id)), bNew(std::exchange(rhs.bNew, false)),
	  bOpen(std::exchange(rhs.bOpen, false)) {
}

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
			ecs::Registry& registry = *pReg;
			registry.attach<T>(entity);
		}
	}
}

template <typename T>
TInspector<T>::operator bool() const {
	return bOpen;
}
} // namespace le::editor
#endif
