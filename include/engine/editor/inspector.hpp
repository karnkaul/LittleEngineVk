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
	using Attach = ktl::move_only_function<T()>;

	template <typename T>
	static std::string_view defaultName() {
		return utils::removeNamespaces(utils::tName<T>());
	}

	template <typename T, typename Insp>
	static void attach(Insp inspector = {}, Attach<T> attach = {}, std::string_view name = defaultName<T>());
	static bool detach(std::string const& id);

  private:
	static void update(SceneRef const& scene);
	static void clear();

	static void attach(dens::entity entity, dens::registry& registry);

	struct GadgetBase {
		virtual ~GadgetBase() = default;
		virtual bool attachable() const noexcept = 0;
		virtual void attach(dens::entity, dens::registry&) const = 0;
		virtual bool inspect(std::string_view, dens::entity, dens::registry&) const { return false; }
		virtual bool inspect(std::string_view, gui::TreeRoot&) const { return false; }
	};
	template <typename T>
	struct TAttach {
		Attach<T> make;
	};
	template <typename T, typename Insp>
	struct TGadgetBase : GadgetBase {
		Insp inspector;
		Attach<T> make;
		TGadgetBase(Insp inspector, Attach<T> make) : inspector(std::move(inspector)), make(std::move(make)) {}
		bool attachable() const noexcept override { return make.has_value(); }
		void attach(dens::entity e, dens::registry& r) const override {
			if (attachable()) { r.attach<T>(e, make()); }
		}
	};
	template <typename T, typename Insp>
	struct TGadget : TGadgetBase<T, Insp> {
		using TGadgetBase<T, Insp>::TGadgetBase;
		bool inspect(std::string_view id, dens::entity e, dens::registry& r) const override;
	};
	template <typename T, typename Insp>
	struct TGuiGadget : TGadgetBase<T, Insp> {
		using TGadgetBase<T, Insp>::TGadgetBase;
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
			auto const detach = ktl::stack_string<64>("Detach##%s", id.data());
			if (Button(detach.get())) { r.detach<T>(e); }
		}
		Styler s(Style::eSeparator);
		return true;
	}
	return false;
}

template <typename T, typename Ins>
bool Inspector::TGuiGadget<T, Ins>::inspect(std::string_view id, gui::TreeRoot& r) const {
	if (auto t = dynamic_cast<T*>(&r)) {
		if (auto tn = TreeNode(id)) {
			this->inspector(Inspect<T>{*t});
			Styler s(Style::eSeparator);
		}
		return true;
	}
	return false;
}

template <typename T, typename Insp>
void Inspector::attach(Insp inspector, Attach<T> attach, std::string_view name) {
	if constexpr (std::is_base_of_v<gui::TreeRoot, T>) {
		s_guiGadgets.insert_or_assign(std::string(name), std::make_unique<TGuiGadget<T, Insp>>(std::move(inspector), std::move(attach)));
	} else {
		s_gadgets.insert_or_assign(std::string(name), std::make_unique<TGadget<T, Insp>>(std::move(inspector), std::move(attach)));
	}
}
} // namespace le::edi
