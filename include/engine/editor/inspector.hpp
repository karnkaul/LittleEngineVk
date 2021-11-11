#pragma once
#include <core/utils/string.hpp>
#include <dens/detail/sign.hpp>
#include <engine/editor/inspect.hpp>
#include <engine/editor/types.hpp>

namespace le::edi {
class Inspector {
  public:
	static constexpr std::string_view title_v = "Inspector";

	template <typename T>
	static std::string_view defaultName() {
		return utils::removeNamespaces(utils::tName<T>());
	}

	template <typename T, typename Insp>
	static void attach(Insp inspector = {}, std::string_view name = defaultName<T>());
	static bool detach(std::string const& id);

  private:
	static void update(SceneRef const& scene);
	static void clear();

	struct GadgetBase {
		virtual ~GadgetBase() = default;
		virtual bool inspect(std::string_view, dens::entity, dens::registry&) const { return false; }
		virtual bool inspect(std::string_view, gui::TreeRoot&) const { return false; }
	};
	template <typename Insp>
	struct TGadgetBase : GadgetBase {
		Insp inspector;
		TGadgetBase(Insp inspector) : inspector(std::move(inspector)) {}
	};
	template <typename T, typename Insp>
	struct TGadget : TGadgetBase<Insp> {
		using TGadgetBase<Insp>::TGadgetBase;
		bool inspect(std::string_view id, dens::entity e, dens::registry& r) const override;
	};
	template <typename T, typename Insp>
	struct TGuiGadget : TGadgetBase<Insp> {
		using TGadgetBase<Insp>::TGadgetBase;
		bool inspect(std::string_view id, gui::TreeRoot& r) const override;
	};

	using GadgetMap = std::unordered_map<std::string, std::unique_ptr<GadgetBase>>;
	inline static GadgetMap s_gadgets;
	inline static GadgetMap s_guiGadgets;
	friend class Sudo;
};

// impl

template <typename T, typename Ins>
bool Inspector::TGadget<T, Ins>::inspect(std::string_view id, dens::entity e, dens::registry& r) const {
	if (auto t = r.find<T>(e)) {
		if (auto tn = TreeNode(id)) {
			this->inspector(Inspect<T>{*t, r, e});
			return true;
		}
	}
	return false;
}

template <typename T, typename Ins>
bool Inspector::TGuiGadget<T, Ins>::inspect(std::string_view id, gui::TreeRoot& r) const {
	if (auto t = dynamic_cast<T*>(&r)) {
		if (auto tn = TreeNode(id)) {
			this->inspector(Inspect<T>{*t});
			return true;
		}
	}
	return false;
}

template <typename T, typename Insp>
void Inspector::attach(Insp inspector, std::string_view name) {
	if constexpr (std::is_base_of_v<gui::TreeRoot, T>) {
		s_guiGadgets.insert_or_assign(std::string(name), std::make_unique<TGuiGadget<T, Insp>>(std::move(inspector)));
	} else {
		s_gadgets.insert_or_assign(std::string(name), std::make_unique<TGadget<T, Insp>>(std::move(inspector)));
	}
}
} // namespace le::edi
